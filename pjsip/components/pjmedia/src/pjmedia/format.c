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
#include <pjmedia/format.h>
#include <pj/pj_assert.h>
#include <pj/pj_errno.h>
#include <pj/pool.h>
#include <pj/pj_string.h>


PJ_DEF(pjmedia_audio_format_detail*)
pjmedia_format_get_audio_format_detail(const pjmedia_format *fmt,
				       pj_bool_t assert_valid)
{
    if (fmt->detail_type==PJMEDIA_FORMAT_DETAIL_AUDIO) {
	return (pjmedia_audio_format_detail*) &fmt->det.aud;
    } else {
        /* Get rid of unused var compiler warning if pj_assert()
         * macro does not do anything
         */
        PJ_UNUSED_ARG(assert_valid);
	pj_assert(!assert_valid || !"Invalid audio format detail");
	return NULL;
    }
}


PJ_DEF(pjmedia_format*) pjmedia_format_copy(pjmedia_format *dst,
					    const pjmedia_format *src)
{
    return (pjmedia_format*)pj_memcpy(dst, src, sizeof(*src));
}
