/* $Id$ */
/*
 * Copyright (C) 2009-2011 Teluu Inc. (http://www.teluu.com)
 * Copyright (C) 2007-2009 Keystream AB and Konftel AB, All rights reserved.
 *                         Author: <dan.aberg@keystream.se>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <pjmedia_audiodev.h>
#include <pj/pj_assert.h>
#include <pj/log.h>
#include <pj/pj_os.h>
#include <pj/pool.h>
#include <pjmedia/errno.h>

#if defined(PJMEDIA_AUDIO_DEV_HAS_ESP) && PJMEDIA_AUDIO_DEV_HAS_ESP

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>
#include <pthread.h>
#include <pj_errno.h>

#include "audio_element.h"
#include "filter_resample.h"
#include "i2s_stream.h"
#include "audio_pipeline.h"
#include <raw_stream.h>

#define THIS_FILE 			        "esp_dev.c"
#define MAX_SOUND_CARDS 		    2
#define MAX_SOUND_DEVICES_PER_CARD 	2
#define MAX_DEVICES			        4
#define MAX_MIX_NAME_LEN            32
#define I2S_SAMPLE_RATE             48000
#define I2S_CHANNELS                2
#define I2S_BITS                    16

#define CODEC_SAMPLE_RATE           8000
#define CODEC_CHANNELS              1

/* Set to 1 to enable tracing */
#define ENABLE_TRACING			1

#if ENABLE_TRACING
#	define TRACE_(expr)		PJ_LOG(5,expr)
#else
#	define TRACE_(expr)
#endif

/*
 * Factory prototypes
 */
static pj_status_t esp_factory_init(pjmedia_aud_dev_factory *f);
static pj_status_t esp_factory_destroy(pjmedia_aud_dev_factory *f);
static pj_status_t esp_factory_refresh(pjmedia_aud_dev_factory *f);
static unsigned    esp_factory_get_dev_count(pjmedia_aud_dev_factory *f);
static pj_status_t esp_factory_get_dev_info(pjmedia_aud_dev_factory *f,
					     unsigned index,
					     pjmedia_aud_dev_info *info);
static pj_status_t esp_factory_default_param(pjmedia_aud_dev_factory *f,
					      unsigned index,
					      pjmedia_aud_param *param);
static pj_status_t esp_factory_create_stream(pjmedia_aud_dev_factory *f,
					      const pjmedia_aud_param *param,
					      pjmedia_aud_rec_cb rec_cb,
					      pjmedia_aud_play_cb play_cb,
					      void *user_data,
					      pjmedia_aud_stream **p_strm);

/*
 * Stream prototypes
 */
static pj_status_t esp_stream_get_param(pjmedia_aud_stream *strm,
					 pjmedia_aud_param *param);
static pj_status_t esp_stream_get_cap(pjmedia_aud_stream *strm,
				       pjmedia_aud_dev_cap cap,
				       void *value);
static pj_status_t esp_stream_set_cap(pjmedia_aud_stream *strm,
				       pjmedia_aud_dev_cap cap,
				       const void *value);
static pj_status_t esp_stream_start(pjmedia_aud_stream *strm);
static pj_status_t esp_stream_stop(pjmedia_aud_stream *strm);
static pj_status_t esp_stream_destroy(pjmedia_aud_stream *strm);


struct esp_factory
{
    pjmedia_aud_dev_factory	 base;
    pj_pool_factory		*pf;
    pj_pool_t			*pool;
    pj_pool_t			*base_pool;

    unsigned			 dev_cnt;
    pjmedia_aud_dev_info	 devs[MAX_DEVICES];
    char                         pb_mixer_name[MAX_MIX_NAME_LEN];
};

struct esp_stream
{
    pjmedia_aud_stream	 base;

    /* Common */
    pj_pool_t		*pool;
    struct esp_factory *af;
    void		*user_data;
    pjmedia_aud_param	 param;		/* Running parameter 		*/
    int                  rec_id;      	/* Capture device id		*/
    int                  quit;

    /* Playback */
    pjmedia_aud_play_cb  pb_cb;
    unsigned             pb_buf_size;
    char		         *pb_buf;
    pj_thread_t		     *pb_thread;
    audio_pipeline_handle_t player;
    audio_element_handle_t raw_write;

    /* Capture */
    pjmedia_aud_rec_cb   ca_cb;
    unsigned             ca_buf_size;
    char		         *ca_buf;
    pj_thread_t		     *ca_thread;
    audio_pipeline_handle_t recorder;
    audio_element_handle_t raw_read;
};

static pjmedia_aud_dev_factory_op esp_factory_op =
{
    &esp_factory_init,
    &esp_factory_destroy,
    &esp_factory_get_dev_count,
    &esp_factory_get_dev_info,
    &esp_factory_default_param,
    &esp_factory_create_stream,
    &esp_factory_refresh
};

static pjmedia_aud_stream_op esp_stream_op =
{
    &esp_stream_get_param,
    &esp_stream_get_cap,
    &esp_stream_set_cap,
    &esp_stream_start,
    &esp_stream_stop,
    &esp_stream_destroy
};

#if ENABLE_TRACING==0
static void null_esp_error_handler (const char *file,
				int line,
				const char *function,
				int err,
				const char *fmt,
				...)
{
    PJ_UNUSED_ARG(file);
    PJ_UNUSED_ARG(line);
    PJ_UNUSED_ARG(function);
    PJ_UNUSED_ARG(err);
    PJ_UNUSED_ARG(fmt);
}
#endif

static pj_status_t add_dev (struct esp_factory *af, const char *dev_name)
{
    pjmedia_aud_dev_info *adi;
    int pb_result, ca_result;

    if (af->dev_cnt >= PJ_ARRAY_SIZE(af->devs))
	return PJ_ETOOMANY;

    adi = &af->devs[af->dev_cnt];

    TRACE_((THIS_FILE, "add_dev (%s): Enter", dev_name));

    /* Reset device info */
    pj_bzero(adi, sizeof(*adi));

    /* Set device name */
    strncpy(adi->name, dev_name, sizeof(adi->name));

    /* Check the number of playback channels */
    adi->output_count = 1;

    /* Check the number of capture channels */
    adi->input_count = 1;

    /* Set the default sample rate */
    adi->default_samples_per_sec = 8000;

    /* Driver name */
    strcpy(adi->driver, "ESP");

    ++af->dev_cnt;

    PJ_LOG (5,(THIS_FILE, "Added sound device %s", adi->name));

    return PJ_SUCCESS;
}

static void get_mixer_name(struct esp_factory *af)
{
   return;
}

/* Create ESP audio driver. */
pjmedia_aud_dev_factory* pjmedia_esp_factory(pj_pool_factory *pf)
{
    struct esp_factory *af;
    pj_pool_t *pool;

    pool = pj_pool_create(pf, "esp_aud_base", 256, 256, NULL);
    af = PJ_POOL_ZALLOC_T(pool, struct esp_factory);
    af->pf = pf;
    af->base_pool = pool;
    af->base.op = &esp_factory_op;

    return &af->base;
}

/* API: init factory */
static pj_status_t esp_factory_init(pjmedia_aud_dev_factory *f)
{
    pj_status_t status = esp_factory_refresh(f);
    if (PJ_SUCCESS != status)
	return status;

    PJ_LOG(4,(THIS_FILE, "ESP initialized"));
    return PJ_SUCCESS;
}


/* API: destroy factory */
static pj_status_t esp_factory_destroy(pjmedia_aud_dev_factory *f)
{
    struct esp_factory *af = (struct esp_factory*)f;

    if (af->pool)
	pj_pool_release(af->pool);

    if (af->base_pool) {
        pj_pool_t *pool = af->base_pool;
        af->base_pool = NULL;
        pj_pool_release(pool);
    }

    return PJ_SUCCESS;
}

/* API: refresh the device list */
static pj_status_t esp_factory_refresh(pjmedia_aud_dev_factory *f)
{
    struct esp_factory *af = (struct esp_factory*)f;

    TRACE_((THIS_FILE, "pjmedia_snd_init: Enumerate sound devices"));

    if (af->pool != NULL) {
        pj_pool_release(af->pool);
        af->pool = NULL;
    }

    af->pool = pj_pool_create(af->pf, "esp_aud", 256, 256, NULL);
    af->dev_cnt = 0;

    /* add sound devices */
    add_dev(af, "esp");

    /* Get the mixer name */
    get_mixer_name(af);

    PJ_LOG(4,(THIS_FILE, "ESP driver found %d devices", af->dev_cnt));

    return PJ_SUCCESS;
}


/* API: get device count */
static unsigned  esp_factory_get_dev_count(pjmedia_aud_dev_factory *f)
{
    struct esp_factory *af = (struct esp_factory*)f;
    return af->dev_cnt;
}

/* API: get device info */
static pj_status_t esp_factory_get_dev_info(pjmedia_aud_dev_factory *f,
					     unsigned index,
					     pjmedia_aud_dev_info *info)
{
    struct esp_factory *af = (struct esp_factory*)f;

    PJ_ASSERT_RETURN(index>=0 && index<af->dev_cnt, PJ_EINVAL);

    pj_memcpy(info, &af->devs[index], sizeof(*info));
    info->caps = PJMEDIA_AUD_DEV_CAP_INPUT_LATENCY |
		 PJMEDIA_AUD_DEV_CAP_OUTPUT_LATENCY;
    return PJ_SUCCESS;
}

/* API: create default parameter */
static pj_status_t esp_factory_default_param(pjmedia_aud_dev_factory *f,
					      unsigned index,
					      pjmedia_aud_param *param)
{
    struct esp_factory *af = (struct esp_factory*)f;
    pjmedia_aud_dev_info *adi;

    PJ_ASSERT_RETURN(index>=0 && index<af->dev_cnt, PJ_EINVAL);

    adi = &af->devs[index];

    pj_bzero(param, sizeof(*param));
    if (adi->input_count && adi->output_count) {
        param->dir = PJMEDIA_DIR_CAPTURE_PLAYBACK;
        param->rec_id = index;
        param->play_id = index;
    } else if (adi->input_count) {
        param->dir = PJMEDIA_DIR_CAPTURE;
        param->rec_id = index;
        param->play_id = PJMEDIA_AUD_INVALID_DEV;
        } else if (adi->output_count) {
        param->dir = PJMEDIA_DIR_PLAYBACK;
        param->play_id = index;
        param->rec_id = PJMEDIA_AUD_INVALID_DEV;
    } else {
        return PJMEDIA_EAUD_INVDEV;
    }

    param->clock_rate = adi->default_samples_per_sec;
    param->channel_count = 1;
    param->samples_per_frame = adi->default_samples_per_sec * 20 / 1000;
    param->bits_per_sample = 16;
    param->flags = adi->caps;
    param->input_latency_ms = PJMEDIA_SND_DEFAULT_REC_LATENCY;
    param->output_latency_ms = PJMEDIA_SND_DEFAULT_PLAY_LATENCY;

    return PJ_SUCCESS;
}

/* playback thtread */
static int pb_thread_func (void *arg)
{
    struct esp_stream* stream = (struct esp_stream*) arg;
    int size                   = stream->pb_buf_size;
    void* user_data            = stream->user_data;
    char* buf 		       = stream->pb_buf;
    pj_timestamp tstamp;
    int result;

    pj_bzero (buf, size);
    tstamp.u64 = 0;

    TRACE_((THIS_FILE, "pb_thread_func(%s): Started", pj_thread_this()->obj_name));

    while (!stream->quit) {
        pjmedia_frame frame;

        frame.type = PJMEDIA_FRAME_TYPE_AUDIO;
        frame.buf = buf;
        frame.size = size;
        frame.timestamp.u64 = tstamp.u64;
        frame.bit_info = 0;

        result = stream->pb_cb (user_data, &frame);
        if (result != PJ_SUCCESS || stream->quit)
            break;

        if (frame.type != PJMEDIA_FRAME_TYPE_AUDIO)
            pj_bzero (buf, size);

        result = raw_stream_write(stream->raw_write, (char *)buf, size);
        if (result < 0) {
            PJ_LOG (4,(THIS_FILE, "pb_thread_func: error writing data!"));
        }
        tstamp.u64 += 160;
    }

    TRACE_((THIS_FILE, "pb_thread_func: Stopped"));
    return PJ_SUCCESS;
}

/* capture thread */
static int ca_thread_func (void *arg)
{
    struct esp_stream* stream = (struct esp_stream*) arg;
    int size                   = stream->ca_buf_size;
    void* user_data            = stream->user_data;
    char* buf 		       = stream->ca_buf;
    pj_timestamp tstamp;
    int result = 0;

    pj_bzero (buf, size);
    tstamp.u64 = 0;

    TRACE_((THIS_FILE, "ca_thread_func(%u): Started", pj_thread_this()->obj_name));

    while (!stream->quit) {
        pjmedia_frame frame;

        pj_bzero (buf, size);
        result = raw_stream_read(stream->raw_read, (char *)buf, size);
        if (result < 0) {
            PJ_LOG (4,(THIS_FILE, "ca_thread_func: error reading data!"));
        }
        if (stream->quit)
            break;

        frame.type = PJMEDIA_FRAME_TYPE_AUDIO;
        frame.buf = (void*) buf;
        frame.size = size;
        frame.timestamp.u64 = tstamp.u64;
        frame.bit_info = 0;

        result = stream->ca_cb (user_data, &frame);
        if (result != PJ_SUCCESS || stream->quit)
            break;

        tstamp.u64 += 160;
    }
    TRACE_((THIS_FILE, "ca_thread_func: Stopped"));

    return PJ_SUCCESS;
}

static pj_status_t open_playback (struct esp_stream* stream,
			          const pjmedia_aud_param *param)
{
    const char *link_tag[3] = {"raw", "filter", "i2s"};
    audio_element_handle_t i2s_stream_writer, filter;
    audio_element_info_t i2s_info = {0};
    unsigned int rate;
    int result;

    /* Open PCM for playback */
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    stream->player = audio_pipeline_init(&pipeline_cfg);
    if (!stream->player)
    {
        PJ_LOG(2, (THIS_FILE, "Speaker created audio pileline failed."));
        return ESP_FAIL;
    }

    raw_stream_cfg_t raw_cfg = RAW_STREAM_CFG_DEFAULT();
    raw_cfg.type = AUDIO_STREAM_WRITER;
    stream->raw_write = raw_stream_init(&raw_cfg);

    rsp_filter_cfg_t rsp_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
    rsp_cfg.src_rate = CODEC_SAMPLE_RATE;
    rsp_cfg.src_ch = CODEC_CHANNELS;
    rsp_cfg.dest_rate = I2S_SAMPLE_RATE;
    rsp_cfg.dest_ch = I2S_CHANNELS;
    rsp_cfg.complexity = 5;
    filter = rsp_filter_init(&rsp_cfg);

    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    i2s_cfg.uninstall_drv = false;
    i2s_stream_writer = i2s_stream_init(&i2s_cfg);
    audio_element_getinfo(i2s_stream_writer, &i2s_info);
    i2s_info.bits = I2S_BITS;
    i2s_info.channels = I2S_CHANNELS;
    i2s_info.sample_rates = I2S_SAMPLE_RATE;
    audio_element_setinfo(i2s_stream_writer, &i2s_info);

    audio_pipeline_register(stream->player, stream->raw_write, "raw");
    audio_pipeline_register(stream->player, filter, "filter");
    audio_pipeline_register(stream->player, i2s_stream_writer, "i2s");
    audio_pipeline_link(stream->player, &link_tag[0], 3);
    // audio_pipeline_run(stream->player);

    /* Set our buffer with size of stream->pb_frames *
        param->channel_count * (param->bits_per_sample/8)*/
    stream->pb_buf_size = 8000 * 20 / 1000 * 1 * 16 / 8;
    stream->pb_buf = (char*) pj_pool_alloc(stream->pool, stream->pb_buf_size);

    PJ_LOG(5, (THIS_FILE, "Speaker has been created"));

    return PJ_SUCCESS;
}

static pj_status_t open_capture (struct esp_stream* stream,
			         const pjmedia_aud_param *param)
{
    audio_element_handle_t i2s_stream_reader;

    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    stream->recorder = audio_pipeline_init(&pipeline_cfg);
    if (!stream->recorder)
    {
        PJ_LOG(2, (THIS_FILE, "recorder created audio pileline failed."));
        return ESP_FAIL;
    }

    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_READER;
    i2s_cfg.uninstall_drv = false;
#if defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
    i2s_cfg.i2s_port = 1;
#endif
    i2s_stream_reader = i2s_stream_init(&i2s_cfg);
    audio_element_info_t i2s_info = {0};
    audio_element_getinfo(i2s_stream_reader, &i2s_info);
    i2s_info.bits = I2S_BITS;
    i2s_info.channels = I2S_CHANNELS;
    i2s_info.sample_rates = I2S_SAMPLE_RATE;
    audio_element_setinfo(i2s_stream_reader, &i2s_info);

    rsp_filter_cfg_t rsp_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
    rsp_cfg.src_rate = I2S_SAMPLE_RATE;
    rsp_cfg.src_ch = I2S_CHANNELS;
    rsp_cfg.dest_rate = CODEC_SAMPLE_RATE;
    rsp_cfg.dest_ch = CODEC_CHANNELS;
    rsp_cfg.complexity = 5;
    audio_element_handle_t filter = rsp_filter_init(&rsp_cfg);

    raw_stream_cfg_t raw_cfg = RAW_STREAM_CFG_DEFAULT();
    raw_cfg.type = AUDIO_STREAM_READER;
    stream->raw_read = raw_stream_init(&raw_cfg);
    audio_element_set_output_timeout(stream->raw_read, portMAX_DELAY);

    audio_pipeline_register(stream->recorder, i2s_stream_reader, "i2s");
    audio_pipeline_register(stream->recorder, filter, "filter");
    audio_pipeline_register(stream->recorder, stream->raw_read, "raw");
    const char *link_tag[3] = {"i2s", "filter", "raw"};
    audio_pipeline_link(stream->recorder, &link_tag[0], 3);
    // audio_pipeline_run(stream->recorder);

    /* Set our buffer with size of stream->ca_frames *
        param->channel_count * (param->bits_per_sample/8)*/
    stream->ca_buf_size = 8000 * 20 / 1000 * 1 * 16 / 8;
    stream->ca_buf = (char*) pj_pool_alloc(stream->pool, stream->ca_buf_size);

    PJ_LOG(4, (THIS_FILE, " SIP recorder has been created"));

    return PJ_SUCCESS;
}

/* API: create stream */
static pj_status_t esp_factory_create_stream(pjmedia_aud_dev_factory *f,
					      const pjmedia_aud_param *param,
					      pjmedia_aud_rec_cb rec_cb,
					      pjmedia_aud_play_cb play_cb,
					      void *user_data,
					      pjmedia_aud_stream **p_strm)
{
    struct esp_factory *af = (struct esp_factory*)f;
    pj_status_t status;
    pj_pool_t* pool;
    struct esp_stream* stream;

    pool = pj_pool_create (af->pf, "esp%p", 1024, 1024, NULL);
    if (!pool)
	return PJ_ENOMEM;

    /* Allocate and initialize comon stream data */
    stream = PJ_POOL_ZALLOC_T (pool, struct esp_stream);
    stream->base.op = &esp_stream_op;
    stream->pool      = pool;
    stream->af 	      = af;
    stream->user_data = user_data;
    stream->pb_cb     = play_cb;
    stream->ca_cb     = rec_cb;
    stream->quit      = 0;
    pj_memcpy(&stream->param, param, sizeof(*param));

    /* Init playback */
    if (param->dir & PJMEDIA_DIR_PLAYBACK) {
        status = open_playback (stream, param);
        if (status != PJ_SUCCESS) {
            pj_pool_release (pool);
            return status;
        }
    }

    /* Init capture */
    if (param->dir & PJMEDIA_DIR_CAPTURE) {
	    status = open_capture (stream, param);
        if (status != PJ_SUCCESS) {
            if (param->dir & PJMEDIA_DIR_PLAYBACK)
            // snd_pcm_close (stream->pb_pcm);
            pj_pool_release (pool);
            return status;
        }
    }

    *p_strm = &stream->base;
    return PJ_SUCCESS;
}

/* API: get running parameter */
static pj_status_t esp_stream_get_param(pjmedia_aud_stream *s,
					 pjmedia_aud_param *pi)
{
    struct esp_stream *stream = (struct esp_stream*)s;

    PJ_ASSERT_RETURN(s && pi, PJ_EINVAL);

    pj_memcpy(pi, &stream->param, sizeof(*pi));

    return PJ_SUCCESS;
}

/* API: get capability */
static pj_status_t esp_stream_get_cap(pjmedia_aud_stream *s,
				       pjmedia_aud_dev_cap cap,
				       void *pval)
{
    struct esp_stream *stream = (struct esp_stream*)s;

    PJ_ASSERT_RETURN(s && pval, PJ_EINVAL);

    if (cap==PJMEDIA_AUD_DEV_CAP_INPUT_LATENCY &&
	    (stream->param.dir & PJMEDIA_DIR_CAPTURE))
    {
        /* Recording latency */
        *(unsigned*)pval = stream->param.input_latency_ms;
        return PJ_SUCCESS;
    } else if (cap==PJMEDIA_AUD_DEV_CAP_OUTPUT_LATENCY &&
	       (stream->param.dir & PJMEDIA_DIR_PLAYBACK))
    {
        /* Playback latency */
        *(unsigned*)pval = stream->param.output_latency_ms;
        return PJ_SUCCESS;
    } else {
	    return PJMEDIA_EAUD_INVCAP;
    }
}

/* API: set capability */
static pj_status_t esp_stream_set_cap(pjmedia_aud_stream *strm,
				       pjmedia_aud_dev_cap cap,
				       const void *value)
{
    struct esp_factory *af = ((struct esp_stream*)strm)->af;

    if (cap==PJMEDIA_AUD_DEV_CAP_OUTPUT_VOLUME_SETTING &&
	    pj_ansi_strlen(af->pb_mixer_name))
    {

	    unsigned vol = *(unsigned*)value;

	    return PJ_SUCCESS;
    }

    return PJMEDIA_EAUD_INVCAP;
}

/* API: start stream */
static pj_status_t esp_stream_start (pjmedia_aud_stream *s)
{
    struct esp_stream *stream = (struct esp_stream*)s;
    pj_status_t status = PJ_SUCCESS;

    stream->quit = 0;
    if (stream->param.dir & PJMEDIA_DIR_PLAYBACK) {
        audio_pipeline_run(stream->player);
        status = pj_thread_create (stream->pool,
                    "espsound_playback",
                    pb_thread_func,
                    stream,
                    0, //ZERO,
                    0,
                    &stream->pb_thread, 19);
        if (status != PJ_SUCCESS)
            return status;
    }

    if (stream->param.dir & PJMEDIA_DIR_CAPTURE) {
        audio_pipeline_run(stream->recorder);
        status = pj_thread_create (stream->pool,
                    "espsound_capture",
                    ca_thread_func,
                    stream,
                    0, //ZERO,
                    0,
                    &stream->ca_thread, 19);
        if (status != PJ_SUCCESS) {
            stream->quit = PJ_TRUE;
            pj_thread_join(stream->pb_thread);
            pj_thread_destroy(stream->pb_thread);
            stream->pb_thread = NULL;
        }
    }

    return status;
}

/* API: stop stream */
static pj_status_t esp_stream_stop (pjmedia_aud_stream *s)
{
    struct esp_stream *stream = (struct esp_stream*)s;

    stream->quit = 1;

    if (stream->pb_thread) {
	    TRACE_((THIS_FILE, "esp_stream_stop(%u): Waiting for playback to stop.",
                    pj_thread_this()->objname));
	    pj_thread_join (stream->pb_thread);
	    TRACE_((THIS_FILE, "esp_stream_stop(%u): playback stopped.",
            pj_thread_this()->objname));
        pj_thread_destroy(stream->pb_thread);
        stream->pb_thread = NULL;

        audio_pipeline_stop(stream->player);
        audio_pipeline_wait_for_stop(stream->player);
    }

    if (stream->ca_thread) {
        TRACE_((THIS_FILE,
            "esp_stream_stop(%u): Waiting for capture to stop.",
            pj_thread_this()->objname));
        pj_thread_join (stream->ca_thread);
        TRACE_((THIS_FILE,
            "esp_stream_stop(%u): capture stopped.",
            pj_thread_this()->objname));
        pj_thread_destroy(stream->ca_thread);
        stream->ca_thread = NULL;

        audio_pipeline_stop(stream->recorder);
        audio_pipeline_wait_for_stop(stream->recorder);
    }

    return PJ_SUCCESS;
}

static pj_status_t esp_stream_destroy (pjmedia_aud_stream *s)
{
    struct esp_stream *stream = (struct esp_stream*)s;

    esp_stream_stop (s);
    if (stream->param.dir & PJMEDIA_DIR_PLAYBACK) {
        // snd_pcm_close (stream->pb_pcm);
        // stream->pb_pcm = NULL;
        audio_pipeline_deinit(stream->player);
    }
    if (stream->param.dir & PJMEDIA_DIR_CAPTURE) {
        // snd_pcm_close (stream->ca_pcm);
        // stream->ca_pcm = NULL;
        audio_pipeline_deinit(stream->recorder);
    }

    pj_pool_release (stream->pool);

    return PJ_SUCCESS;
}

#endif	/* PJMEDIA_AUDIO_DEV_HAS_ESP */
