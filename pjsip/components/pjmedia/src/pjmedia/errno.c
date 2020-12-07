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
#include <pjmedia/errno.h>
#include <pjmedia/media_types.h>
#include <pj/pj_string.h>
#if PJMEDIA_AUDIO_DEV_HAS_PORTAUDIO
#   include <portaudio.h>
#endif

#if defined(PJMEDIA_HAS_SRTP) && (PJMEDIA_HAS_SRTP != 0)
PJ_BEGIN_DECL
    char * get_libsrtp_errstr(int err);
PJ_END_DECL
#endif

/*
 * pjmedia_strerror()
 */
PJ_DEF(pj_str_t) pjmedia_strerror( pj_status_t statcode, 
				   char *buf, pj_size_t bufsize )
{
    pj_str_t errstr;

#if defined(PJ_HAS_ERROR_STRING) && (PJ_HAS_ERROR_STRING != 0)

    /* See if the error comes from PortAudio. */
#if PJMEDIA_AUDIO_DEV_HAS_PORTAUDIO
    if (statcode >= PJMEDIA_PORTAUDIO_ERRNO_START &&
	statcode <= PJMEDIA_PORTAUDIO_ERRNO_END)
    {

	//int pa_err = statcode - PJMEDIA_ERRNO_FROM_PORTAUDIO(0);
	int pa_err = PJMEDIA_PORTAUDIO_ERRNO_START - statcode;
	pj_str_t msg;
	
	msg.ptr = (char*)Pa_GetErrorText(pa_err);
	msg.slen = pj_ansi_strlen(msg.ptr);

	errstr.ptr = buf;
	pj_strncpy_with_null(&errstr, &msg, bufsize);
	return errstr;

    } else 
#endif	/* PJMEDIA_AUDIO_DEV_HAS_PORTAUDIO */

#if defined(PJMEDIA_HAS_SRTP) && (PJMEDIA_HAS_SRTP != 0)
    /* LIBSRTP error */
    if (statcode >= PJMEDIA_LIBSRTP_ERRNO_START &&
	statcode <  PJMEDIA_LIBSRTP_ERRNO_END)
    {
	int err = statcode - PJMEDIA_LIBSRTP_ERRNO_START;
	pj_str_t msg;
	
	msg = pj_str((char*)get_libsrtp_errstr(err));

	errstr.ptr = buf;
	pj_strncpy_with_null(&errstr, &msg, bufsize);
    if(msg.ptr)
        pj_free(msg.ptr);
	return errstr;
    
    } else
#endif
    
    /* PJMEDIA error */
    if (statcode >= PJMEDIA_ERRNO_START && 
	       statcode < PJMEDIA_ERRNO_END)
    {
	/* Find the error in the table.
	 * Use binary search!
	 */
	int first = 0;
	int n = PJ_ARRAY_SIZE(err_str);

	while (n > 0) {
	    int half = n/2;
	    int mid = first + half;

	    if (err_str[mid].code < statcode) {
		first = mid+1;
		n -= (half+1);
	    } else if (err_str[mid].code > statcode) {
		n = half;
	    } else {
		first = mid;
		break;
	    }
	}


	if (PJ_ARRAY_SIZE(err_str) && err_str[first].code == statcode) {
	    pj_str_t msg;
	    
	    msg.ptr = (char*)err_str[first].msg;
	    msg.slen = pj_ansi_strlen(err_str[first].msg);

	    errstr.ptr = buf;
	    pj_strncpy_with_null(&errstr, &msg, bufsize);
	    return errstr;

	} 
    } 
#endif	/* PJ_HAS_ERROR_STRING */

    /* Error not found. */
    errstr.ptr = buf;
    errstr.slen = pj_ansi_snprintf(buf, bufsize, 
				   "Unknown pjmedia error %d",
				   statcode);
    if (errstr.slen < 1 || errstr.slen >= (pj_ssize_t)bufsize)
	errstr.slen = bufsize - 1;
    return errstr;
}

