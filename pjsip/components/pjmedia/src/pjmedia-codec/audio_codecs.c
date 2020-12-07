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
#include <pjmedia-codec.h>

PJ_DEF(pj_status_t)
pjmedia_codec_register_audio_codecs(pjmedia_endpt *endpt)
{
    pj_status_t status = PJ_SUCCESS;

    PJ_ASSERT_RETURN(endpt, PJ_EINVAL);

#if PJMEDIA_HAS_G711_CODEC
    /* Register PCMA and PCMU */
    status = pjmedia_codec_g711_init(endpt);
    if (status != PJ_SUCCESS)
	return status;
#endif	/* PJMEDIA_HAS_G711_CODEC */

    return PJ_SUCCESS;
}

