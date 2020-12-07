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
#include <pj/pj_os.h>
#include <pj/pj_ctype.h>
#include <pj/pj_errno.h>
#include <pj/pj_string.h>

/*
 * FYI these links contain useful infos about predefined macros across
 * platforms:
 *  - http://predef.sourceforge.net/preos.html
 */

#if defined(PJ_HAS_SYS_UTSNAME_H) && PJ_HAS_SYS_UTSNAME_H != 0
/* For uname() */
#   include <sys/utsname.h>
#   include <stdlib.h>
#   define PJ_HAS_UNAME		1
#endif

#if defined(PJ_HAS_LIMITS_H) && PJ_HAS_LIMITS_H != 0
/* Include <limits.h> to get <features.h> to get various glibc macros.
 * See http://predef.sourceforge.net/prelib.html
 */
#   include <limits.h>
#endif

#ifndef PJ_SYS_INFO_BUFFER_SIZE
#   define PJ_SYS_INFO_BUFFER_SIZE	64
#endif


static char *ver_info(pj_uint32_t ver, char *buf)
{
    pj_size_t len;

    if (ver == 0) {
		*buf = '\0';
		return buf;
    }

    sprintf(buf, "-%u.%u",
	    (ver & 0xFF000000) >> 24,
	    (ver & 0x00FF0000) >> 16);
    len = strlen(buf);

    if (ver & 0xFFFF) {
		sprintf(buf+len, ".%u", (ver & 0xFF00) >> 8);
		len = strlen(buf);

		if (ver & 0x00FF) {
			sprintf(buf+len, ".%u", (ver & 0xFF));
		}
    }

    return buf;
}

static pj_uint32_t parse_version(char *str)
{    
    int i, maxtok;
    pj_ssize_t found_idx;
    pj_uint32_t version = 0;
    pj_str_t in_str = pj_str(str);
    pj_str_t token, delim;
    
    while (*str && !pj_isdigit(*str))
		str++;

    maxtok = 4;
    delim = pj_str(".-");
    for (found_idx = pj_strtok(&in_str, &delim, &token, 0), i=0; 
	 found_idx != in_str.slen && i < maxtok;
	 ++i, found_idx = pj_strtok(&in_str, &delim, &token, 
	                            found_idx + token.slen))
    {
		int n;

		if (!pj_isdigit(*token.ptr))
			break;
		
		n = atoi(token.ptr);
		version |= (n << ((3-i)*8));
    }
    
    return version;
}

PJ_DEF(pj_sys_info*) pj_get_sys_info(void)
{
    char * si_buffer = (char *)pj_calloc(PJ_SYS_INFO_BUFFER_SIZE, sizeof(char));
    pj_sys_info *si = (pj_sys_info *)pj_calloc(1, sizeof(pj_sys_info));
    pj_size_t left = PJ_SYS_INFO_BUFFER_SIZE, len;

    si->machine.ptr = si->os_name.ptr = si->sdk_name.ptr = si->info.ptr = "";

#define ALLOC_CP_STR(str,field)	\
	do { \
	    len = pj_ansi_strlen(str); \
	    if (len && left >= len+1) { \
		si->field.ptr = si_buffer + PJ_SYS_INFO_BUFFER_SIZE - left; \
		si->field.slen = len; \
		pj_memcpy(si->field.ptr, str, len+1); \
		left -= (len+1); \
	    } \
	} while (0)

#if defined(ESP_PLATFORM) && ESP_PLATFORM != 0
    {
    	si->machine = pj_str("XTENSA");
    	si->os_name = pj_str("ESP32");
    	si->os_ver = "3.3";
    	si->sdk_name = pj_str("3.3");
    	si->sdk_ver = "3.3";
    }
#endif

#if defined(PJ_HAS_UNAME) && PJ_HAS_UNAME
    /*
     * SDK info.
     */
get_sdk_info:
#endif

#if defined(__GLIBC__)
    si.sdk_ver = (__GLIBC__ << 24) |
		 (__GLIBC_MINOR__ << 16);
    si.sdk_name = pj_str("glibc");
#elif defined(__GNU_LIBRARY__)
    si.sdk_ver = (__GNU_LIBRARY__ << 24) |
	         (__GNU_LIBRARY_MINOR__ << 16);
    si.sdk_name = pj_str("libc");
#elif defined(__UCLIBC__)
    si.sdk_ver = (__UCLIBC_MAJOR__ << 24) |
    	         (__UCLIBC_MINOR__ << 16);
    si.sdk_name = pj_str("uclibc");
#endif

    /*
     * Build the info string.
     */
    {
	char tmp[PJ_SYS_INFO_BUFFER_SIZE];
	char os_ver[20], sdk_ver[20];
	int cnt;

	cnt = pj_ansi_snprintf(tmp, sizeof(tmp),
			       "%s%s%s%s%s%s%s",
			       si->os_name.ptr,
			       ver_info(si->os_ver, os_ver),
			       (si->machine.slen ? "/" : ""),
			       si->machine.ptr,
			       (si->sdk_name.slen ? "/" : ""),
			       si->sdk_name.ptr,
			       ver_info(si->sdk_ver, sdk_ver));
	if (cnt > 0 && cnt < (int)sizeof(tmp)) {
	    ALLOC_CP_STR(tmp, info);
	}
    }
    return si;
}
