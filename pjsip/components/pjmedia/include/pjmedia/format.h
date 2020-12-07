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
#ifndef __PJMEDIA_FORMAT_H__
#define __PJMEDIA_FORMAT_H__

/**
 * @file pjmedia/format.h Media format
 * @brief Media format
 */
#include <pjmedia/media_types.h>

/**
 * @defgroup PJMEDIA_FORMAT Media format
 * @ingroup PJMEDIA_TYPES
 * @brief Media format
 * @{
 */

PJ_BEGIN_DECL

/**
 * Macro for packing format from a four character code, similar to FOURCC.
 * This macro is used for building the constants in pjmedia_format_id
 * enumeration.
 */
#define PJMEDIA_FORMAT_PACK(C1, C2, C3, C4) PJMEDIA_FOURCC(C1, C2, C3, C4)

/**
 * This enumeration uniquely identify audio sample and/or video pixel formats.
 * Some well known formats are listed here. The format ids are built by
 * combining four character codes, similar to FOURCC. The format id is
 * extensible, as application may define and use format ids not declared
 * on this enumeration.
 *
 * This format id along with other information will fully describe the media
 * in #pjmedia_format structure.
 */
typedef enum pjmedia_format_id
{
    /*
     * Audio formats
     */

    /** 16bit signed integer linear PCM audio */
    PJMEDIA_FORMAT_L16	    = 0,

    /** Alias for PJMEDIA_FORMAT_L16 */
    PJMEDIA_FORMAT_PCM	    = PJMEDIA_FORMAT_L16,

    /** G.711 ALAW */
    PJMEDIA_FORMAT_PCMA	    = PJMEDIA_FORMAT_PACK('A', 'L', 'A', 'W'),

    /** Alias for PJMEDIA_FORMAT_PCMA */
    PJMEDIA_FORMAT_ALAW	    = PJMEDIA_FORMAT_PCMA,

    /** G.711 ULAW */
    PJMEDIA_FORMAT_PCMU	    = PJMEDIA_FORMAT_PACK('u', 'L', 'A', 'W'),

    /** Aliaw for PJMEDIA_FORMAT_PCMU */
    PJMEDIA_FORMAT_ULAW	    = PJMEDIA_FORMAT_PCMU,

    /** AMR narrowband */
    PJMEDIA_FORMAT_AMR	    = PJMEDIA_FORMAT_PACK(' ', 'A', 'M', 'R'),

    /** ITU G.729 */
    PJMEDIA_FORMAT_G729	    = PJMEDIA_FORMAT_PACK('G', '7', '2', '9'),

    /** Internet Low Bit-Rate Codec (ILBC) */
    PJMEDIA_FORMAT_ILBC	    = PJMEDIA_FORMAT_PACK('I', 'L', 'B', 'C'),

    PJMEDIA_FORMAT_INVALID  = 0xFFFFFFFF

} pjmedia_format_id;

/**
 * This enumeration specifies what type of detail is included in a
 * #pjmedia_format structure.
 */
typedef enum pjmedia_format_detail_type
{
    /** Format detail is not specified. */
    PJMEDIA_FORMAT_DETAIL_NONE,

    /** Audio format detail. */
    PJMEDIA_FORMAT_DETAIL_AUDIO,

    /** Number of format detail type that has been defined. */
    PJMEDIA_FORMAT_DETAIL_MAX

} pjmedia_format_detail_type;

/**
 * This structure is put in \a detail field of #pjmedia_format to describe
 * detail information about an audio media.
 */
typedef struct pjmedia_audio_format_detail
{
    unsigned	clock_rate;	/**< Audio clock rate in samples or Hz. */
    unsigned	channel_count;	/**< Number of channels.		*/
    unsigned	frame_time_usec;/**< Frame interval, in microseconds.	*/
    unsigned	bits_per_sample;/**< Number of bits per sample.		*/
    pj_uint32_t	avg_bps;	/**< Average bitrate			*/
    pj_uint32_t	max_bps;	/**< Maximum bitrate			*/
} pjmedia_audio_format_detail;

/**
 * This macro declares the size of the detail section in #pjmedia_format
 * to be reserved for user defined detail.
 */
#ifndef PJMEDIA_FORMAT_DETAIL_USER_SIZE
#   define PJMEDIA_FORMAT_DETAIL_USER_SIZE		1
#endif

/**
 * This structure contains all the information needed to completely describe
 * a media.
 */
typedef struct pjmedia_format
{
    /**
     * The format id that specifies the audio sample or video pixel format.
     * Some well known formats ids are declared in pjmedia_format_id
     * enumeration.
     *
     * @see pjmedia_format_id
     */
    pj_uint32_t		 	 id;

    /**
     * The top-most type of the media, as an information.
     */
    pjmedia_type		 type;

    /**
     * The type of detail structure in the \a detail pointer.
     */
    pjmedia_format_detail_type	 detail_type;

    /**
     * Detail section to describe the media.
     */
    union
    {
	/**
	 * Detail section for audio format.
	 */
	pjmedia_audio_format_detail	aud;

	/**
	 * Reserved area for user-defined format detail.
	 */
	char				user[PJMEDIA_FORMAT_DETAIL_USER_SIZE];
    } det;

} pjmedia_format;

/*****************************************************************************
 * UTILITIES:
 */

/**
 * General utility routine to calculate samples per frame value from clock
 * rate, ptime (in usec), and channel count. Application should use this
 * macro whenever possible due to possible overflow in the math calculation.
 *
 * @param clock_rate		Clock rate.
 * @param usec_ptime		Frame interval, in microsecond.
 * @param channel_count		Number of channels.
 *
 * @return			The samples per frame value.
 */
PJ_INLINE(unsigned) PJMEDIA_SPF(unsigned clock_rate, unsigned usec_ptime,
				unsigned channel_count)
{
#if PJ_HAS_INT64
    return ((unsigned)((pj_uint64_t)usec_ptime * \
		       clock_rate * channel_count / 1000000));
#elif PJ_HAS_FLOATING_POINT
    return ((unsigned)(1.0*usec_ptime * clock_rate * channel_count / 1000000));
#else
    return ((unsigned)(usec_ptime / 1000L * clock_rate * \
		       channel_count / 1000));
#endif
}

/**
 * Variant of #PJMEDIA_SPF() which takes frame rate instead of ptime.
 */
PJ_INLINE(unsigned) PJMEDIA_SPF2(unsigned clock_rate, const pjmedia_ratio *fr,
				 unsigned channel_count)
{
#if PJ_HAS_INT64
    return ((unsigned)((pj_uint64_t)clock_rate * fr->denum \
		       / fr->num / channel_count));
#elif PJ_HAS_FLOATING_POINT
    return ((unsigned)(1.0* clock_rate * fr->denum / fr->num /channel_count));
#else
    return ((unsigned)(1L * clock_rate * fr->denum / fr->num / channel_count));
#endif
}


/**
 * Utility routine to calculate frame size (in bytes) from bitrate and frame
 * interval values. Application should use this macro whenever possible due
 * to possible overflow in the math calculation.
 *
 * @param bps			The bitrate of the stream.
 * @param usec_ptime		Frame interval, in microsecond.
 *
 * @return			Frame size in bytes.
 */
PJ_INLINE(unsigned) PJMEDIA_FSZ(unsigned bps, unsigned usec_ptime)
{
#if PJ_HAS_INT64
    return ((unsigned)((pj_uint64_t)bps * usec_ptime / PJ_UINT64(8000000)));
#elif PJ_HAS_FLOATING_POINT
    return ((unsigned)(1.0 * bps * usec_ptime / 8000000.0));
#else
    return ((unsigned)(bps / 8L * usec_ptime / 1000000));
#endif
}

/**
 * General utility routine to calculate ptime value from frame rate.
 * Application should use this macro whenever possible due to possible
 * overflow in the math calculation.
 *
 * @param frame_rate		Frame rate
 *
 * @return			The ptime value (in usec).
 */
PJ_INLINE(unsigned) PJMEDIA_PTIME(const pjmedia_ratio *frame_rate)
{
#if PJ_HAS_INT64
    return ((unsigned)((pj_uint64_t)1000000 * \
		       frame_rate->denum / frame_rate->num));
#elif PJ_HAS_FLOATING_POINT
    return ((unsigned)(1000000.0 * frame_rate->denum /
                       frame_rate->num));
#else
    return ((unsigned)((1000L * frame_rate->denum /
                       frame_rate->num) * 1000));
#endif
}

/**
 * Utility to retrieve samples_per_frame value from
 * pjmedia_audio_format_detail.
 *
 * @param pafd		Pointer to pjmedia_audio_format_detail
 * @return		Samples per frame
 */
PJ_INLINE(unsigned) PJMEDIA_AFD_SPF(const pjmedia_audio_format_detail *pafd)
{
    return PJMEDIA_SPF(pafd->clock_rate, pafd->frame_time_usec,
		       pafd->channel_count);
}

/**
 * Utility to retrieve average frame size from pjmedia_audio_format_detail.
 * The average frame size is derived from the average bitrate of the audio
 * stream.
 *
 * @param afd		Pointer to pjmedia_audio_format_detail
 * @return		Average frame size.
 */
PJ_INLINE(unsigned) PJMEDIA_AFD_AVG_FSZ(const pjmedia_audio_format_detail *afd)
{
    return PJMEDIA_FSZ(afd->avg_bps, afd->frame_time_usec);
}

/**
 * Utility to retrieve maximum frame size from pjmedia_audio_format_detail.
 * The maximum frame size is derived from the maximum bitrate of the audio
 * stream.
 *
 * @param afd		Pointer to pjmedia_audio_format_detail
 * @return		Average frame size.
 */
PJ_INLINE(unsigned) PJMEDIA_AFD_MAX_FSZ(const pjmedia_audio_format_detail *afd)
{
    return PJMEDIA_FSZ(afd->max_bps, afd->frame_time_usec);
}


/**
 * Initialize the format as audio format with the specified parameters.
 *
 * @param fmt			The format to be initialized.
 * @param fmt_id		Format ID. See #pjmedia_format_id
 * @param clock_rate		Audio clock rate.
 * @param channel_count		Number of channels.
 * @param bits_per_sample	Number of bits per sample.
 * @param frame_time_usec	Frame interval, in microsecond.
 * @param avg_bps		Average bitrate.
 * @param max_bps		Maximum bitrate.
 */
PJ_INLINE(void) pjmedia_format_init_audio(pjmedia_format *fmt,
				          pj_uint32_t fmt_id,
					  unsigned clock_rate,
					  unsigned channel_count,
					  unsigned bits_per_sample,
					  unsigned frame_time_usec,
					  pj_uint32_t avg_bps,
					  pj_uint32_t max_bps)
{
    /* This function is inlined to avoid build problem due to circular
     * dependency, i.e: this function is part of pjmedia and is needed
     * by pjmedia-audiodev, while pjmedia depends on pjmedia-audiodev.
     */

    fmt->id = fmt_id;
    fmt->type = PJMEDIA_TYPE_AUDIO;
    fmt->detail_type = PJMEDIA_FORMAT_DETAIL_AUDIO;

    fmt->det.aud.clock_rate = clock_rate;
    fmt->det.aud.channel_count = channel_count;
    fmt->det.aud.bits_per_sample = bits_per_sample;
    fmt->det.aud.frame_time_usec = frame_time_usec;
    fmt->det.aud.avg_bps = avg_bps;
    fmt->det.aud.max_bps = max_bps;
}

/**
 * Copy format to another.
 *
 * @param dst		The destination format.
 * @param src		The source format.
 *
 * @return		Pointer to destination format.
 */
PJ_DECL(pjmedia_format*) pjmedia_format_copy(pjmedia_format *dst,
					     const pjmedia_format *src);

/**
 * Check if the format contains audio format, and retrieve the audio format
 * detail in the format.
 *
 * @param fmt		The format structure.
 * @param assert_valid	If this is set to non-zero, an assertion will be
 * 			raised if the detail type is not audio or if the
 * 			the detail is NULL.
 *
 * @return		The instance of audio format detail in the format
 * 			structure, or NULL if the format doesn't contain
 * 			audio detail.
 */
PJ_DECL(pjmedia_audio_format_detail*)
pjmedia_format_get_audio_format_detail(const pjmedia_format *fmt,
				       pj_bool_t assert_valid);

PJ_END_DECL

/**
 * @}
 */

#endif	/* __PJMEDIA_FORMAT_H__ */

