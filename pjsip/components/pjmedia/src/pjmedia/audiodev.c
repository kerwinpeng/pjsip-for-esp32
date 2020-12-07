/* $Id$ */
/* 
 * Copyright (C) 2008-2011 Teluu Inc. (http://www.teluu.com)
 * Copyright (C) 2003-2008 Benny Prijono <benny@prijono.org>
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
#include <pjmedia-audiodev/audiodev_imp.h>
#include <pj/pj_assert.h>
#include <pj/pj_errno.h>
#include <pj/log.h>
#include <pj/pool.h>
#include <pj/pj_string.h>

#define THIS_FILE   "audiodev.c"

#define DEFINE_CAP(name, info)	{name, info}

/* Capability names */
static struct cap_info
{
    const char *name;
    const char *info;
} cap_infos[] = 
{
    DEFINE_CAP("ext-fmt",     "Extended/non-PCM format"),
    DEFINE_CAP("latency-in",  "Input latency/buffer size setting"),
    DEFINE_CAP("latency-out", "Output latency/buffer size setting"),
    DEFINE_CAP("vol-in",      "Input volume setting"),
    DEFINE_CAP("vol-out",     "Output volume setting"),
    DEFINE_CAP("meter-in",    "Input meter"),
    DEFINE_CAP("meter-out",   "Output meter"),
    DEFINE_CAP("route-in",    "Input routing"),
    DEFINE_CAP("route-out",   "Output routing"),
    DEFINE_CAP("aec",	      "Accoustic echo cancellation"),
    DEFINE_CAP("aec-tail",    "Tail length setting for AEC"),
    DEFINE_CAP("vad",	      "Voice activity detection"),
    DEFINE_CAP("cng",	      "Comfort noise generation"),
    DEFINE_CAP("plg",	      "Packet loss concealment")
};

/*
 * The device index seen by application and driver is different. 
 *
 * At application level, device index is index to global list of device.
 * At driver level, device index is index to device list on that particular
 * factory only.
 */
#define MAKE_DEV_ID(f_id, index)   (((f_id & 0xFFFF) << 16) | (index & 0xFFFF))
#define GET_INDEX(dev_id)	   ((dev_id) & 0xFFFF)
#define GET_FID(dev_id)		   ((dev_id) >> 16)
#define DEFAULT_DEV_ID		    0

/* The audio subsystem */
static pjmedia_aud_subsys aud_subsys;

/* API: get the audio subsystem. */
PJ_DEF(pjmedia_aud_subsys*) pjmedia_get_aud_subsys(void)
{
    return &aud_subsys;
}

/* API: init driver */
PJ_DEF(pj_status_t) pjmedia_aud_driver_init(unsigned drv_idx,
					    pj_bool_t refresh)
{
    pjmedia_aud_driver *drv = &aud_subsys.drv[drv_idx];
    pjmedia_aud_dev_factory *f;
    unsigned i, dev_cnt;
    pj_status_t status;

    if (!refresh && drv->create) {
	/* Create the factory */
	f = (*drv->create)(aud_subsys.pf);
	if (!f)
	    return PJ_EUNKNOWN;

	/* Call factory->init() */
	status = f->op->init(f);
	if (status != PJ_SUCCESS) {
	    f->op->destroy(f);
	    return status;
	}
    } else {
	f = drv->f;
    }

    if (!f)
	return PJ_EUNKNOWN;

    /* Get number of devices */
    dev_cnt = f->op->get_dev_count(f);
    if (dev_cnt + aud_subsys.dev_cnt > PJMEDIA_AUD_MAX_DEVS) {
	PJ_LOG(4,(THIS_FILE, "%d device(s) cannot be registered because"
			      " there are too many devices",
			      aud_subsys.dev_cnt + dev_cnt -
			      PJMEDIA_AUD_MAX_DEVS));
	dev_cnt = PJMEDIA_AUD_MAX_DEVS - aud_subsys.dev_cnt;
    }

    /* enabling this will cause pjsua-lib initialization to fail when there
     * is no sound device installed in the system, even when pjsua has been
     * run with --null-audio
     *
    if (dev_cnt == 0) {
	f->op->destroy(f);
	return PJMEDIA_EAUD_NODEV;
    }
    */

    /* Fill in default devices */
    drv->play_dev_idx = drv->rec_dev_idx =
			drv->dev_idx = PJMEDIA_AUD_INVALID_DEV;
    for (i=0; i<dev_cnt; ++i) {
	pjmedia_aud_dev_info info;

	status = f->op->get_dev_info(f, i, &info);
	if (status != PJ_SUCCESS) {
	    f->op->destroy(f);
	    return status;
	}

	if (drv->name[0]=='\0') {
	    /* Set driver name */
	    pj_ansi_strncpy(drv->name, info.driver, sizeof(drv->name));
	    drv->name[sizeof(drv->name)-1] = '\0';
	}

	if (drv->play_dev_idx < 0 && info.output_count) {
	    /* Set default playback device */
	    drv->play_dev_idx = i;
	}
	if (drv->rec_dev_idx < 0 && info.input_count) {
	    /* Set default capture device */
	    drv->rec_dev_idx = i;
	}
	if (drv->dev_idx < 0 && info.input_count &&
	    info.output_count)
	{
	    /* Set default capture and playback device */
	    drv->dev_idx = i;
	}

	if (drv->play_dev_idx >= 0 && drv->rec_dev_idx >= 0 && 
	    drv->dev_idx >= 0) 
	{
	    /* Done. */
	    break;
	}
    }

    /* Register the factory */
    drv->f = f;
    drv->f->sys.drv_idx = drv_idx;
    drv->start_idx = aud_subsys.dev_cnt;
    drv->dev_cnt = dev_cnt;

    /* Register devices to global list */
    for (i=0; i<dev_cnt; ++i) {
	aud_subsys.dev_list[aud_subsys.dev_cnt++] = MAKE_DEV_ID(drv_idx, i);
    }

    return PJ_SUCCESS;
}

/* API: deinit driver */
PJ_DEF(void) pjmedia_aud_driver_deinit(unsigned drv_idx)
{
    pjmedia_aud_driver *drv = &aud_subsys.drv[drv_idx];

    if (drv->f) {
	drv->f->op->destroy(drv->f);
	drv->f = NULL;
    }

    pj_bzero(drv, sizeof(*drv));
    drv->play_dev_idx = drv->rec_dev_idx = 
			drv->dev_idx = PJMEDIA_AUD_INVALID_DEV;
}

/* API: get capability name/info */
PJ_DEF(const char*) pjmedia_aud_dev_cap_name(pjmedia_aud_dev_cap cap,
					     const char **p_desc)
{
    const char *desc;
    unsigned i;

    if (p_desc==NULL) p_desc = &desc;

    for (i=0; i<PJ_ARRAY_SIZE(cap_infos); ++i) {
	if ((1 << i)==cap)
	    break;
    }

    if (i==PJ_ARRAY_SIZE(cap_infos)) {
	*p_desc = "??";
	return "??";
    }

    *p_desc = cap_infos[i].info;
    return cap_infos[i].name;
}

static pj_status_t get_cap_pointer(const pjmedia_aud_param *param,
				   pjmedia_aud_dev_cap cap,
				   void **ptr,
				   unsigned *size)
{
#define FIELD_INFO(name)    *ptr = (void*)&param->name; \
			    *size = sizeof(param->name)

    switch (cap) {
    case PJMEDIA_AUD_DEV_CAP_EXT_FORMAT:
	FIELD_INFO(ext_fmt);
	break;
    case PJMEDIA_AUD_DEV_CAP_INPUT_LATENCY:
	FIELD_INFO(input_latency_ms);
	break;
    case PJMEDIA_AUD_DEV_CAP_OUTPUT_LATENCY:
	FIELD_INFO(output_latency_ms);
	break;
    case PJMEDIA_AUD_DEV_CAP_INPUT_VOLUME_SETTING:
	FIELD_INFO(input_vol);
	break;
    case PJMEDIA_AUD_DEV_CAP_OUTPUT_VOLUME_SETTING:
	FIELD_INFO(output_vol);
	break;
    case PJMEDIA_AUD_DEV_CAP_INPUT_ROUTE:
	FIELD_INFO(input_route);
	break;
    case PJMEDIA_AUD_DEV_CAP_OUTPUT_ROUTE:
	FIELD_INFO(output_route);
	break;
    case PJMEDIA_AUD_DEV_CAP_EC:
	FIELD_INFO(ec_enabled);
	break;
    case PJMEDIA_AUD_DEV_CAP_EC_TAIL:
	FIELD_INFO(ec_tail_ms);
	break;
    /* vad is no longer in "fmt" in 2.0.
    case PJMEDIA_AUD_DEV_CAP_VAD:
	FIELD_INFO(ext_fmt.vad);
	break;
    */
    case PJMEDIA_AUD_DEV_CAP_CNG:
	FIELD_INFO(cng_enabled);
	break;
    case PJMEDIA_AUD_DEV_CAP_PLC:
	FIELD_INFO(plc_enabled);
	break;
    default:
	return PJMEDIA_EAUD_INVCAP;
    }

#undef FIELD_INFO

    return PJ_SUCCESS;
}

/* API: set cap value to param */
PJ_DEF(pj_status_t) pjmedia_aud_param_set_cap( pjmedia_aud_param *param,
					       pjmedia_aud_dev_cap cap,
					       const void *pval)
{
    void *cap_ptr;
    unsigned cap_size;
    pj_status_t status;

    status = get_cap_pointer(param, cap, &cap_ptr, &cap_size);
    if (status != PJ_SUCCESS)
	return status;

    pj_memcpy(cap_ptr, pval, cap_size);
    param->flags |= cap;

    return PJ_SUCCESS;
}

/* API: get cap value from param */
PJ_DEF(pj_status_t) pjmedia_aud_param_get_cap( const pjmedia_aud_param *param,
					       pjmedia_aud_dev_cap cap,
					       void *pval)
{
    void *cap_ptr;
    unsigned cap_size;
    pj_status_t status;

    status = get_cap_pointer(param, cap, &cap_ptr, &cap_size);
    if (status != PJ_SUCCESS)
	return status;

    if ((param->flags & cap) == 0) {
	pj_bzero(cap_ptr, cap_size);
	return PJMEDIA_EAUD_INVCAP;
    }

    pj_memcpy(pval, cap_ptr, cap_size);
    return PJ_SUCCESS;
}

/* API: Get device information. */
PJ_DEF(pj_status_t) pjmedia_aud_dev_get_info(pjmedia_aud_dev_index id,
					     pjmedia_aud_dev_info *info)
{
    pjmedia_aud_dev_factory *f;
    unsigned index = 0;
    pj_status_t status;

    PJ_ASSERT_RETURN(info && id!=PJMEDIA_AUD_INVALID_DEV, PJ_EINVAL);
    PJ_ASSERT_RETURN(aud_subsys.pf, PJMEDIA_EAUD_INIT);

    f = aud_subsys.drv[0].f;

    return f->op->get_dev_info(f, index, info);
}

/* API: Open audio stream object using the specified parameters. */
PJ_DEF(pj_status_t) pjmedia_aud_stream_create(const pjmedia_aud_param *prm,
					      pjmedia_aud_rec_cb rec_cb,
					      pjmedia_aud_play_cb play_cb,
					      void *user_data,
					      pjmedia_aud_stream **p_aud_strm)
{
    pjmedia_aud_dev_factory *f=NULL;
    pjmedia_aud_param param;
    pj_status_t status;

    PJ_ASSERT_RETURN(prm && prm->dir && p_aud_strm, PJ_EINVAL);
    PJ_ASSERT_RETURN(prm->dir==PJMEDIA_DIR_CAPTURE ||
		     prm->dir==PJMEDIA_DIR_PLAYBACK ||
		     prm->dir==PJMEDIA_DIR_CAPTURE_PLAYBACK,
		     PJ_EINVAL);

    /* Must make copy of param because we're changing device ID */
    pj_memcpy(&param, prm, sizeof(param));

    /* Normalize rec_id */

    /* Normalize play_id */

    printf("pjmedia_aud_stream_create param.dir=%d\n", param.dir);
    f = aud_subsys.drv[0].f;
    PJ_ASSERT_RETURN(f != NULL, PJ_EBUG);

    // /* For now, rec_id and play_id must belong to the same factory */
    // PJ_ASSERT_RETURN((param.dir != PJMEDIA_DIR_CAPTURE_PLAYBACK),
	// 	     PJMEDIA_EAUD_INVDEV);

    /* Create the stream */
    status = f->op->create_stream(f, &param, rec_cb, play_cb,
				  user_data, p_aud_strm);
    if (status != PJ_SUCCESS)
	return status;

    /* Assign factory id to the stream */
    (*p_aud_strm)->sys.drv_idx = f->sys.drv_idx;
        return PJ_SUCCESS;
}

/* API: Get the running parameters for the specified audio stream. */
PJ_DEF(pj_status_t) pjmedia_aud_stream_get_param(pjmedia_aud_stream *strm,
						 pjmedia_aud_param *param)
{
    pj_status_t status;

    PJ_ASSERT_RETURN(strm && param, PJ_EINVAL);

    status = strm->op->get_param(strm, param);
    if (status != PJ_SUCCESS)
	return status;

    /* Normalize device id's */

    return PJ_SUCCESS;
}

/* API: Get the value of a specific capability of the audio stream. */
PJ_DEF(pj_status_t) pjmedia_aud_stream_get_cap(pjmedia_aud_stream *strm,
					       pjmedia_aud_dev_cap cap,
					       void *value)
{
    return strm->op->get_cap(strm, cap, value);
}

/* API: Set the value of a specific capability of the audio stream. */
PJ_DEF(pj_status_t) pjmedia_aud_stream_set_cap(pjmedia_aud_stream *strm,
					       pjmedia_aud_dev_cap cap,
					       const void *value)
{
    return strm->op->set_cap(strm, cap, value);
}

/* API: Start the stream. */
PJ_DEF(pj_status_t) pjmedia_aud_stream_start(pjmedia_aud_stream *strm)
{
    return strm->op->start(strm);
}

/* API: Stop the stream. */
PJ_DEF(pj_status_t) pjmedia_aud_stream_stop(pjmedia_aud_stream *strm)
{
    return strm->op->stop(strm);
}

/* API: Destroy the stream. */
PJ_DEF(pj_status_t) pjmedia_aud_stream_destroy(pjmedia_aud_stream *strm)
{
    return strm->op->destroy(strm);
}


