/* $Id$ */
/* 
 * Copyright (C) 2011-2011 Teluu Inc. (http://www.teluu.com)
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
#ifndef __PJMEDIA_CODEC_ALL_CODECS_H__
#define __PJMEDIA_CODEC_ALL_CODECS_H__

/**
 * @file pjmedia-codec/all_codecs.h
 * @brief Helper function to register all codecs
 */
#include <pjmedia/endpoint.h>

PJ_BEGIN_DECL

/**
 * @defgroup PJMEDIA_CODEC_REGISTER_ALL Codec registration helper
 * @ingroup PJMEDIA_CODEC_CODECS
 * @brief Helper function to register all codecs
 * @{
 *
 * Helper function to register all codecs that are implemented in
 * PJMEDIA-CODEC library.
 */

/**
 * Register all known audio codecs implemented in PJMEDA-CODEC library to the
 * specified media endpoint.
 *
 * @param endpt		The media endpoint.
 *
 * @return		PJ_SUCCESS on success or the appropriate error code.
 */
PJ_DECL(pj_status_t)
pjmedia_codec_register_audio_codecs(pjmedia_endpt *endpt);


/**
 * @}  PJMEDIA_CODEC_REGISTER_ALL
 */


PJ_END_DECL

#endif	/* __PJMEDIA_CODEC_ALL_CODECS_H__ */
