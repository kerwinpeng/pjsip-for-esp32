/* $Id$ */
/* 
 * Copyright (C) 2003-2007 Benny Prijono <benny@prijono.org>
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
#ifndef __SRTP_CONFIG_H__
#define __SRTP_CONFIG_H__

#include <pj/pj_types.h>

/* We'll just define CISC if it's x86 family */
#if defined (PJ_M_I386) || defined(_i386_) || defined(i_386_) || \
    defined(_X86_) || defined(x86) || defined(__i386__) || \
    defined(__i386) || defined(_M_IX86) || defined(__I86__) || \
    defined (PJ_M_X86_64) || defined(__amd64__) || defined(__amd64) || \
    defined(__x86_64__) || defined(__x86_64) || \
    defined(PJ_M_IA64) || defined(__ia64__) || defined(_IA64) || \
    defined(__IA64__) || defined(_M_IA64)
#   define CPU_CISC	    1
/* #   define HAVE_X86	    1   use X86 inlined assembly code */
#else
/*#   define CPU_RISC	    1*/
#   define CPU_CISC	    1
#endif

/* Define to compile in dynamic debugging system. */
#define ENABLE_DEBUGGING    PJ_DEBUG

/* Define to 1 if you have the <arpa/inet.h> header file. */
#if defined(PJ_HAS_ARPA_INET_H) && PJ_HAS_ARPA_INET_H!=0
#   define HAVE_ARPA_INET_H 1
#endif

/* Define to 1 if you have the <byteswap.h> header file. */
/* #undef HAVE_BYTESWAP_H */

/* Define to 1 if you have the `inet_aton' function. */
#if defined(PJ_SOCK_HAS_INET_PTON) && PJ_SOCK_HAS_INET_PTON
#   define HAVE_INET_ATON   1
#endif


/* Define to 1 if you have the <netinet/in.h> header file. */
#if defined(PJ_HAS_NETINET_IN_H) && PJ_HAS_NETINET_IN_H!=0
#   define HAVE_NETINET_IN_H	1
#endif

/* Define to 1 if you have the <stdlib.h> header file. */
#if defined(PJ_HAS_STDLIB_H) && PJ_HAS_STDLIB_H!=0
#   define HAVE_STDLIB_H    1
#endif

/* Define to 1 if you have the <string.h> header file. */
#if defined(PJ_HAS_STRING_H) && PJ_HAS_STRING_H!=0
#   define HAVE_STRING_H    1
#endif

/* Define to 1 if you have the <sys/socket.h> header file. */
#if defined(PJ_HAS_SYS_SOCKET_H) && PJ_HAS_SYS_SOCKET_H!=0
#   define HAVE_SYS_SOCKET_H	1
#endif

/* Define to 1 if you have the <sys/types.h> header file. */
#if defined(PJ_HAS_SYS_TYPES_H) && PJ_HAS_SYS_TYPES_H!=0
#   define HAVE_SYS_TYPES_H 1
#endif

/* Define to 1 if you have the <unistd.h> header file. */
/* Define to 1 if you have the `usleep' function. */
#if defined(PJ_HAS_UNISTD_H) && PJ_HAS_UNISTD_H!=0
#   define HAVE_UNISTD_H    1
#   define HAVE_USLEEP	    1
#endif

#define HAVE_INT16_T	    1
#define HAVE_INT32_T	    1
#define HAVE_INT8_T	    1
#define HAVE_UINT8_T	    1
#define HAVE_UINT16_T	    1
#define HAVE_UINT32_T	    1
#define HAVE_UINT64_T	    1

/* Define to 1 if you have the <stdint.h> header file. */
#if defined(PJ_HAS_STDINT_H) && PJ_HAS_STDINT_H!=0
#   define HAVE_STDINT_H    1
#else
    typedef pj_uint8_t	    uint8_t;
    typedef pj_uint16_t	    uint16_t;
    typedef pj_uint32_t	    uint32_t;
    typedef pj_uint64_t	    uint64_t;
    typedef pj_int8_t	    int8_t;
    typedef pj_int16_t	    int16_t;
    typedef pj_int32_t	    int32_t;
    typedef pj_int64_t	    int64_t;
#endif

/* These shouldn't really matter as long as HAVE_UINT64_T is set */
#define SIZEOF_UNSIGNED_LONG	    (sizeof(unsigned long))
#define SIZEOF_UNSIGNED_LONG_LONG   8


/* Path to random device */
/* #define DEV_URANDOM "/dev/urandom" */

/* Only with PJSIP:
 * Try to open PJ_DEV_URANDOM if present
 */
#if defined(PJ_HAS_FCNTL_H) && defined(PJ_HAS_UNISTD_H)
#   define PJ_DEV_URANDOM	"/dev/urandom"
#endif

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "libsrtp 2.1.0"

/* Define to the version of this package. */
#define PACKAGE_VERSION "2.1.0"


#endif	/* __SRTP_CONFIG_H__ */

