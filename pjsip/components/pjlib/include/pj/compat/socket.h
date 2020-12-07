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
#ifndef __PJ_COMPAT_SOCKET_H__
#define __PJ_COMPAT_SOCKET_H__

/**
 * @file socket.h
 * @brief Provides all socket related functions,data types, error codes, etc.
 */

#if defined(PJ_HAS_SYS_TYPES_H) && PJ_HAS_SYS_TYPES_H != 0
#  include <sys/types.h>
#endif

#if defined(PJ_HAS_SYS_SOCKET_H) && PJ_HAS_SYS_SOCKET_H != 0
#  include <sys/socket.h>
#   define PJ_SOCK_HAS_GETADDRINFO	1
#endif

#if defined(PJ_HAS_LINUX_SOCKET_H) && PJ_HAS_LINUX_SOCKET_H != 0
#  include <linux/socket.h>
#endif

#if defined(PJ_HAS_SYS_SELECT_H) && PJ_HAS_SYS_SELECT_H != 0
#  include <sys/select.h>
#endif

#if defined(PJ_HAS_NETINET_IN_H) && PJ_HAS_NETINET_IN_H != 0
#  include <netinet/in.h>
#endif

#if defined(PJ_HAS_NETINET_IN_SYSTM_H) && PJ_HAS_NETINET_IN_SYSTM_H != 0
/* Required to include netinet/ip.h in FreeBSD 7.0 */
#  include <netinet/in_systm.h>
#endif

#if defined(PJ_HAS_NETINET_IP_H) && PJ_HAS_NETINET_IP_H != 0
/* To pull in IPTOS_* constants */
#  include <netinet/ip.h>
#endif

#if defined(PJ_HAS_NETINET_TCP_H) && PJ_HAS_NETINET_TCP_H != 0
/* To pull in TCP_NODELAY constants */
#  include <netinet/tcp.h>
#endif

#if defined(PJ_HAS_NET_IF_H) && PJ_HAS_NET_IF_H != 0
/* For interface enumeration in ip_helper */
#   include <net/if.h>
#endif

#if defined(PJ_HAS_IFADDRS_H) && PJ_HAS_IFADDRS_H != 0
/* Interface enum with getifaddrs() which works with IPv6 */
#   include <ifaddrs.h>
#endif

#if defined(PJ_HAS_ARPA_INET_H) && PJ_HAS_ARPA_INET_H != 0
#  include <arpa/inet.h>
#endif

#if defined(PJ_HAS_SYS_IOCTL_H) && PJ_HAS_SYS_IOCTL_H != 0
#  include <sys/ioctl.h>	/* FBIONBIO */
#endif

#if defined(PJ_HAS_ERRNO_H) && PJ_HAS_ERRNO_H != 0
#  include <pj_errno.h>
#endif

#if defined(PJ_HAS_NETDB_H) && PJ_HAS_NETDB_H != 0
#  include <netdb.h>
#endif

#if defined(PJ_HAS_UNISTD_H) && PJ_HAS_UNISTD_H != 0
#  include <unistd.h>
#endif

#if defined(PJ_HAS_SYS_FILIO_H) && PJ_HAS_SYS_FILIO_H != 0
#   include <sys/filio.h>
#endif

#if defined(PJ_HAS_SYS_SOCKIO_H) && PJ_HAS_SYS_SOCKIO_H != 0
#   include <sys/sockio.h>
#endif


/*
 * Define common errors.
 */
#if (defined(PJ_WIN32) && PJ_WIN32!=0) || \
    (defined(PJ_WIN32_WINCE) && PJ_WIN32_WINCE!=0) || \
    (defined(PJ_WIN64) && PJ_WIN64!=0)
#  define OSERR_EWOULDBLOCK    WSAEWOULDBLOCK
#  define OSERR_EINPROGRESS    WSAEINPROGRESS
#  define OSERR_ECONNRESET     WSAECONNRESET
#  define OSERR_ENOTCONN       WSAENOTCONN
#  define OSERR_EAFNOSUPPORT   WSAEAFNOSUPPORT
#  define OSERR_ENOPROTOOPT    WSAENOPROTOOPT
#elif defined(PJ_SYMBIAN) && PJ_SYMBIAN!=0
#  define OSERR_EWOULDBLOCK    -1
#  define OSERR_EINPROGRESS    -1
#  define OSERR_ECONNRESET     -1
#  define OSERR_ENOTCONN       -1
#  define OSERR_EAFNOSUPPORT   -1
#  define OSERR_ENOPROTOOPT    -1
#else
#  define OSERR_EWOULDBLOCK    EWOULDBLOCK
#  define OSERR_EINPROGRESS    EINPROGRESS
#  define OSERR_ECONNRESET     ECONNRESET
#  define OSERR_ENOTCONN       ENOTCONN
#  define OSERR_EAFNOSUPPORT   EAFNOSUPPORT
#  define OSERR_ENOPROTOOPT    ENOPROTOOPT
#endif


/*
 * And undefine these..
 * Note (see issue #2311): unfortunately, this may cause build failure
 * to anyone who uses these standard macros.
 */
//#undef s_addr
//#undef s6_addr
//#undef sin_zero

/*
 * This will finally be obsoleted, since it should be declared in
 * os_auto.h
 */
#if !defined(PJ_HAS_SOCKLEN_T) || PJ_HAS_SOCKLEN_T==0
    typedef int socklen_t;
#endif

/* Regarding sin_len member of sockaddr_in:
 *  BSD systems (including MacOS X requires that the sin_len member of 
 *  sockaddr_in be set to sizeof(sockaddr_in), while other systems (Windows
 *  and Linux included) do not.
 *
 *  To maintain compatibility between systems, PJLIB will automatically
 *  set this field before invoking native OS socket API, and it will
 *  always reset the field to zero before returning pj_sockaddr_in to
 *  application (such as in pj_getsockname() and pj_recvfrom()).
 *
 *  Application MUST always set this field to zero.
 *
 *  This way we can avoid hard to find problem such as when the socket 
 *  address is used as hash table key.
 */
#if defined(PJ_SOCKADDR_HAS_LEN) && PJ_SOCKADDR_HAS_LEN!=0
#   define PJ_SOCKADDR_SET_LEN(addr,len) (((pj_addr_hdr*)(addr))->sa_zero_len=(len))
#   define PJ_SOCKADDR_RESET_LEN(addr)   (((pj_addr_hdr*)(addr))->sa_zero_len=0)
#else
#   define PJ_SOCKADDR_SET_LEN(addr,len) 
#   define PJ_SOCKADDR_RESET_LEN(addr)
#endif

#endif	/* __PJ_COMPAT_SOCKET_H__ */

