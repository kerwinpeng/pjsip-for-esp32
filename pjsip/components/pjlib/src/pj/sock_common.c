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
#include <pj/sock.h>
#include <pj/pj_assert.h>
#include <pj/pj_ctype.h>
#include <pj/pj_errno.h>
#include <pj/ip_helper.h>
#include <pj/pj_os.h>
#include <pj/addr_resolv.h>
#include <pj/rand.h>
#include <pj/pj_string.h>
#include <pj/compat/socket.h>
#include "mdns.h"

#if 0
    /* Enable some tracing */
    #include <pj/log.h>
    #define THIS_FILE   "sock_common.c"
    #define TRACE_(arg)	PJ_LOG(4,arg)
#else
    #define TRACE_(arg)
#endif


/*
 * Convert address string with numbers and dots to binary IP address.
 */ 
PJ_DEF(pj_in_addr) pj_inet_addr(const pj_str_t *cp)
{
    pj_in_addr addr;

    pj_inet_aton(cp, &addr);
    return addr;
}

/*
 * Convert address string with numbers and dots to binary IP address.
 */ 
PJ_DEF(pj_in_addr) pj_inet_addr2(const char *cp)
{
    pj_str_t str = pj_str((char*)cp);
    return pj_inet_addr(&str);
}

/*
 * Get text representation.
 */
PJ_DEF(char*) pj_inet_ntop2( int af, const void *src,
			     char *dst, int size)
{
    pj_status_t status;

    status = pj_inet_ntop(af, src, dst, size);
    return (status==PJ_SUCCESS)? dst : NULL;
}

/*
 * Print socket address.
 */
PJ_DEF(char*) pj_sockaddr_print( const pj_sockaddr_t *addr,
				 char *buf, int size,
				 unsigned flags)
{
    enum {
	WITH_PORT = 1,
	WITH_BRACKETS = 2
    };

    char txt[PJ_INET6_ADDRSTRLEN];
    char port[32];
    const pj_addr_hdr *h = (const pj_addr_hdr*)addr;
    char *bquote, *equote;
    pj_status_t status;

    status = pj_inet_ntop(h->sa_family, pj_sockaddr_get_addr(addr),
			  txt, sizeof(txt));
    if (status != PJ_SUCCESS)
	return "";

    if (h->sa_family != PJ_AF_INET6 || (flags & WITH_BRACKETS)==0) {
	bquote = ""; equote = "";
    } else {
	bquote = "["; equote = "]";
    }

    if (flags & WITH_PORT) {
	pj_ansi_snprintf(port, sizeof(port), ":%d",
			 pj_sockaddr_get_port(addr));
    } else {
	port[0] = '\0';
    }

    pj_ansi_snprintf(buf, size, "%s%s%s%s",
		     bquote, txt, equote, port);

    return buf;
}

/*
 * Set the IP address of an IP socket address from string address, 
 * with resolving the host if necessary. The string address may be in a
 * standard numbers and dots notation or may be a hostname. If hostname
 * is specified, then the function will resolve the host into the IP
 * address.
 */
PJ_DEF(pj_status_t) pj_sockaddr_in_set_str_addr( pj_sockaddr_in *addr,
					         const pj_str_t *str_addr)
{
    PJ_CHECK_STACK();

    PJ_ASSERT_RETURN(!str_addr || str_addr->slen < PJ_MAX_HOSTNAME, 
                     (addr->sin_addr.s_addr=PJ_INADDR_NONE, PJ_EINVAL));

    PJ_SOCKADDR_RESET_LEN(addr);
    addr->sin_family = PJ_AF_INET;
    pj_bzero(addr->sin_zero_pad, sizeof(addr->sin_zero_pad));

    if (str_addr && str_addr->slen) {
	addr->sin_addr = pj_inet_addr(str_addr);
	if (addr->sin_addr.s_addr == PJ_INADDR_NONE) {
    	    pj_addrinfo ai;
	    unsigned count = 1;
	    pj_status_t status;

	    status = pj_getaddrinfo(pj_AF_INET(), str_addr, &count, &ai);
	    if (status==PJ_SUCCESS) {
		pj_memcpy(&addr->sin_addr, &ai.ai_addr.ipv4.sin_addr,
			  sizeof(addr->sin_addr));
	    } else {
		return status;
	    }
	}

    } else {
	addr->sin_addr.s_addr = 0;
    }

    return PJ_SUCCESS;
}

/* Set address from a name */
PJ_DEF(pj_status_t) pj_sockaddr_set_str_addr(int af,
					     pj_sockaddr *addr,
					     const pj_str_t *str_addr)
{
    pj_status_t status;

    if (af == PJ_AF_INET) {
	return pj_sockaddr_in_set_str_addr(&addr->ipv4, str_addr);
    }

    PJ_ASSERT_RETURN(af==PJ_AF_INET6, PJ_EAFNOTSUP);

    /* IPv6 specific */

    addr->ipv6.sin6_family = PJ_AF_INET6;
    PJ_SOCKADDR_RESET_LEN(addr);

    if (str_addr && str_addr->slen) {
#if defined(PJ_SOCKADDR_USE_GETADDRINFO) && PJ_SOCKADDR_USE_GETADDRINFO!=0
	if (1) {
#else
	status = pj_inet_pton(PJ_AF_INET6, str_addr, &addr->ipv6.sin6_addr);
	if (status != PJ_SUCCESS) {
#endif
    	    pj_addrinfo ai;
	    unsigned count = 1;

	    status = pj_getaddrinfo(PJ_AF_INET6, str_addr, &count, &ai);
	    if (status==PJ_SUCCESS) {
		pj_memcpy(&addr->ipv6.sin6_addr, &ai.ai_addr.ipv6.sin6_addr,
			  sizeof(addr->ipv6.sin6_addr));
		addr->ipv6.sin6_scope_id = ai.ai_addr.ipv6.sin6_scope_id;
	    }
	}
    } else {
	status = PJ_SUCCESS;
    }

    return status;
}

/*
 * Set the IP address and port of an IP socket address.
 * The string address may be in a standard numbers and dots notation or 
 * may be a hostname. If hostname is specified, then the function will 
 * resolve the host into the IP address.
 */
PJ_DEF(pj_status_t) pj_sockaddr_in_init( pj_sockaddr_in *addr,
				         const pj_str_t *str_addr,
					 pj_uint16_t port)
{
    PJ_ASSERT_RETURN(addr, (addr->sin_addr.s_addr=PJ_INADDR_NONE, PJ_EINVAL));

    PJ_SOCKADDR_RESET_LEN(addr);
    addr->sin_family = PJ_AF_INET;
    pj_bzero(addr->sin_zero_pad, sizeof(addr->sin_zero_pad));
    pj_sockaddr_in_set_port(addr, port);
    return pj_sockaddr_in_set_str_addr(addr, str_addr);
}

/*
 * Initialize IP socket address based on the address and port info.
 */
PJ_DEF(pj_status_t) pj_sockaddr_init(int af, 
				     pj_sockaddr *addr,
				     const pj_str_t *cp,
				     pj_uint16_t port)
{
    pj_status_t status;

    if (af == PJ_AF_INET) {
	return pj_sockaddr_in_init(&addr->ipv4, cp, port);
    }

    /* IPv6 specific */
    PJ_ASSERT_RETURN(af==PJ_AF_INET6, PJ_EAFNOTSUP);

    pj_bzero(addr, sizeof(pj_sockaddr_in6));
    addr->addr.sa_family = PJ_AF_INET6;
    
    status = pj_sockaddr_set_str_addr(af, addr, cp);
    if (status != PJ_SUCCESS)
	return status;

    addr->ipv6.sin6_port = pj_htons(port);
    return PJ_SUCCESS;
}

/*
 * Compare two socket addresses.
 */
PJ_DEF(int) pj_sockaddr_cmp( const pj_sockaddr_t *addr1,
			     const pj_sockaddr_t *addr2)
{
    const pj_sockaddr *a1 = (const pj_sockaddr*) addr1;
    const pj_sockaddr *a2 = (const pj_sockaddr*) addr2;
    int port1, port2;
    int result;

    /* Compare address family */
    if (a1->addr.sa_family < a2->addr.sa_family)
	return -1;
    else if (a1->addr.sa_family > a2->addr.sa_family)
	return 1;

    /* Compare addresses */
    result = pj_memcmp(pj_sockaddr_get_addr(a1),
		       pj_sockaddr_get_addr(a2),
		       pj_sockaddr_get_addr_len(a1));
    if (result != 0)
	return result;

    /* Compare port number */
    port1 = pj_sockaddr_get_port(a1);
    port2 = pj_sockaddr_get_port(a2);

    if (port1 < port2)
	return -1;
    else if (port1 > port2)
	return 1;

    /* TODO:
     *	Do we need to compare flow label and scope id in IPv6? 
     */
    
    /* Looks equal */
    return 0;
}

/*
 * Get port number of a pj_sockaddr_in
 */
PJ_DEF(pj_uint16_t) pj_sockaddr_in_get_port(const pj_sockaddr_in *addr)
{
    return pj_ntohs(addr->sin_port);
}

/*
 * Get the address part
 */
PJ_DEF(void*) pj_sockaddr_get_addr(const pj_sockaddr_t *addr)
{
    const pj_sockaddr *a = (const pj_sockaddr*)addr;

    PJ_ASSERT_RETURN(a->addr.sa_family == PJ_AF_INET ||
		     a->addr.sa_family == PJ_AF_INET6, NULL);

    if (a->addr.sa_family == PJ_AF_INET6)
	return (void*) &a->ipv6.sin6_addr;
    else
	return (void*) &a->ipv4.sin_addr;
}

/*
 * Check if sockaddr contains a non-zero address
 */
PJ_DEF(pj_bool_t) pj_sockaddr_has_addr(const pj_sockaddr_t *addr)
{
    const pj_sockaddr *a = (const pj_sockaddr*)addr;

    /* It's probably not wise to raise assertion here if
     * the address doesn't contain a valid address family, and
     * just return PJ_FALSE instead.
     * 
     * The reason is because application may need to distinguish 
     * these three conditions with sockaddr:
     *	a) sockaddr is not initialized. This is by convention
     *	   indicated by sa_family==0.
     *	b) sockaddr is initialized with zero address. This is
     *	   indicated with the address field having zero address.
     *	c) sockaddr is initialized with valid address/port.
     *
     * If we enable this assertion, then application will loose
     * the capability to specify condition a), since it will be
     * forced to always initialize sockaddr (even with zero address).
     * This may break some parts of upper layer libraries.
     */
    //PJ_ASSERT_RETURN(a->addr.sa_family == PJ_AF_INET ||
    //		     a->addr.sa_family == PJ_AF_INET6, PJ_FALSE);

    if (a->addr.sa_family!=PJ_AF_INET && a->addr.sa_family!=PJ_AF_INET6) {
	return PJ_FALSE;
    } else if (a->addr.sa_family == PJ_AF_INET6) {
	pj_uint8_t zero[24];
	pj_bzero(zero, sizeof(zero));
	return pj_memcmp(a->ipv6.sin6_addr.s6_addr, zero, 
			 sizeof(pj_in6_addr)) != 0;
    } else
	return a->ipv4.sin_addr.s_addr != PJ_INADDR_ANY;
}

/*
 * Get port number
 */
PJ_DEF(pj_uint16_t) pj_sockaddr_get_port(const pj_sockaddr_t *addr)
{
    const pj_sockaddr *a = (const pj_sockaddr*) addr;

    PJ_ASSERT_RETURN(a->addr.sa_family == PJ_AF_INET ||
		     a->addr.sa_family == PJ_AF_INET6, (pj_uint16_t)0xFFFF);

    return pj_ntohs((pj_uint16_t)(a->addr.sa_family == PJ_AF_INET6 ?
				    a->ipv6.sin6_port : a->ipv4.sin_port));
}

/*
 * Get the length of the address part.
 */
PJ_DEF(unsigned) pj_sockaddr_get_addr_len(const pj_sockaddr_t *addr)
{
    const pj_sockaddr *a = (const pj_sockaddr*) addr;
    PJ_ASSERT_RETURN(a->addr.sa_family == PJ_AF_INET ||
		     a->addr.sa_family == PJ_AF_INET6, 0);
    return a->addr.sa_family == PJ_AF_INET6 ?
	    sizeof(pj_in6_addr) : sizeof(pj_in_addr);
}

/*
 * Get socket address length.
 */
PJ_DEF(unsigned) pj_sockaddr_get_len(const pj_sockaddr_t *addr)
{
    const pj_sockaddr *a = (const pj_sockaddr*) addr;
    PJ_ASSERT_RETURN(a->addr.sa_family == PJ_AF_INET ||
		     a->addr.sa_family == PJ_AF_INET6, 0);
    return a->addr.sa_family == PJ_AF_INET6 ?
	    sizeof(pj_sockaddr_in6) : sizeof(pj_sockaddr_in);
}

/*
 * Copy only the address part (sin_addr/sin6_addr) of a socket address.
 */
PJ_DEF(void) pj_sockaddr_copy_addr( pj_sockaddr *dst,
				    const pj_sockaddr *src)
{
    /* Destination sockaddr might not be initialized */
    const char *srcbuf = (char*)pj_sockaddr_get_addr(src);
    char *dstbuf = ((char*)dst) + (srcbuf - (char*)src);
    pj_memcpy(dstbuf, srcbuf, pj_sockaddr_get_addr_len(src));
}

/*
 * Copy socket address.
 */
PJ_DEF(void) pj_sockaddr_cp(pj_sockaddr_t *dst, const pj_sockaddr_t *src)
{
    pj_memcpy(dst, src, pj_sockaddr_get_len(src));
}

/*
 * Synthesize address.
 */
PJ_DEF(pj_status_t) pj_sockaddr_synthesize(int dst_af,
				           pj_sockaddr_t *dst,
				           const pj_sockaddr_t *src)
{
    char ip_addr_buf[PJ_INET6_ADDRSTRLEN];
    unsigned int count = 1;
    pj_addrinfo ai[1];
    pj_str_t ip_addr;
    pj_status_t status;

    /* Validate arguments */
    PJ_ASSERT_RETURN(src && dst, PJ_EINVAL);

    if (dst_af == ((const pj_sockaddr *)src)->addr.sa_family) {
        pj_sockaddr_cp(dst, src);
        return PJ_SUCCESS;
    }

    pj_sockaddr_print(src, ip_addr_buf, sizeof(ip_addr_buf), 0);
    ip_addr = pj_str(ip_addr_buf);
    
    /* Try to synthesize address using pj_getaddrinfo(). */
    status = pj_getaddrinfo(dst_af, &ip_addr, &count, ai); 
    if (status == PJ_SUCCESS && count > 0) {
    	pj_sockaddr_cp(dst, &ai[0].ai_addr);
    	pj_sockaddr_set_port(dst, pj_sockaddr_get_port(src));
    }
    
    return status;
}

/*
 * Set port number of pj_sockaddr_in
 */
PJ_DEF(void) pj_sockaddr_in_set_port(pj_sockaddr_in *addr, 
				     pj_uint16_t hostport)
{
    addr->sin_port = pj_htons(hostport);
}

/*
 * Set port number of pj_sockaddr
 */
PJ_DEF(pj_status_t) pj_sockaddr_set_port(pj_sockaddr *addr, 
					 pj_uint16_t hostport)
{
    int af = addr->addr.sa_family;

    PJ_ASSERT_RETURN(af==PJ_AF_INET || af==PJ_AF_INET6, PJ_EINVAL);

    if (af == PJ_AF_INET6)
	addr->ipv6.sin6_port = pj_htons(hostport);
    else
	addr->ipv4.sin_port = pj_htons(hostport);

    return PJ_SUCCESS;
}

/*
 * Get IPv4 address
 */
PJ_DEF(pj_in_addr) pj_sockaddr_in_get_addr(const pj_sockaddr_in *addr)
{
    pj_in_addr in_addr;
    in_addr.s_addr = pj_ntohl(addr->sin_addr.s_addr);
    return in_addr;
}

/*
 * Set IPv4 address
 */
PJ_DEF(void) pj_sockaddr_in_set_addr(pj_sockaddr_in *addr,
				     pj_uint32_t hostaddr)
{
    addr->sin_addr.s_addr = pj_htonl(hostaddr);
}

/*
 * Parse address
 */
PJ_DEF(pj_status_t) pj_sockaddr_parse2(int af, unsigned options,
				       const pj_str_t *str,
				       pj_str_t *p_hostpart,
				       pj_uint16_t *p_port,
				       int *raf)
{
    const char *end = str->ptr + str->slen;
    const char *last_colon_pos = NULL;
    unsigned colon_cnt = 0;
    const char *p;

    PJ_ASSERT_RETURN((af==PJ_AF_INET || af==PJ_AF_INET6 || af==PJ_AF_UNSPEC) &&
		     options==0 &&
		     str!=NULL, PJ_EINVAL);

    /* Special handling for empty input */
    if (str->slen==0 || str->ptr==NULL) {
	if (p_hostpart)
	    p_hostpart->slen = 0;
	if (p_port)
	    *p_port = 0;
	if (raf)
	    *raf = PJ_AF_INET;
	return PJ_SUCCESS;
    }

    /* Count the colon and get the last colon */
    for (p=str->ptr; p!=end; ++p) {
	if (*p == ':') {
	    ++colon_cnt;
	    last_colon_pos = p;
	}
    }

    /* Deduce address family if it's not given */
    if (af == PJ_AF_UNSPEC) {
	if (colon_cnt > 1)
	    af = PJ_AF_INET6;
	else
	    af = PJ_AF_INET;
    } else if (af == PJ_AF_INET && colon_cnt > 1)
	return PJ_EINVAL;

    if (raf)
	*raf = af;

    if (af == PJ_AF_INET) {
	/* Parse as IPv4. Supported formats:
	 *  - "10.0.0.1:80"
	 *  - "10.0.0.1"
	 *  - "10.0.0.1:"
	 *  - ":80"
	 *  - ":"
	 */
	pj_str_t hostpart;
	unsigned long port;

	hostpart.ptr = (char*)str->ptr;

	if (last_colon_pos) {
	    pj_str_t port_part;
	    int i;

	    hostpart.slen = last_colon_pos - str->ptr;

	    port_part.ptr = (char*)last_colon_pos + 1;
	    port_part.slen = end - port_part.ptr;

	    /* Make sure port number is valid */
	    for (i=0; i<port_part.slen; ++i) {
		if (!pj_isdigit(port_part.ptr[i]))
		    return PJ_EINVAL;
	    }
	    port = pj_strtoul(&port_part);
	    if (port > 65535)
		return PJ_EINVAL;
	} else {
	    hostpart.slen = str->slen;
	    port = 0;
	}

	if (p_hostpart)
	    *p_hostpart = hostpart;
	if (p_port)
	    *p_port = (pj_uint16_t)port;

	return PJ_SUCCESS;

    } else if (af == PJ_AF_INET6) {

	/* Parse as IPv6. Supported formats:
	 *  - "fe::01:80"  ==> note: port number is zero in this case, not 80!
	 *  - "[fe::01]:80"
	 *  - "fe::01"
	 *  - "fe::01:"
	 *  - "[fe::01]"
	 *  - "[fe::01]:"
	 *  - "[::]:80"
	 *  - ":::80"
	 *  - "[::]"
	 *  - "[::]:"
	 *  - ":::"
	 *  - "::"
	 */
	pj_str_t hostpart, port_part;

	if (*str->ptr == '[') {
	    char *end_bracket;
	    int i;
	    unsigned long port;

	    if (last_colon_pos == NULL)
		return PJ_EINVAL;

	    end_bracket = pj_strchr(str, ']');
	    if (end_bracket == NULL)
		return PJ_EINVAL;

	    hostpart.ptr = (char*)str->ptr + 1;
	    hostpart.slen = end_bracket - hostpart.ptr;

	    if (last_colon_pos < end_bracket) {
		port_part.ptr = NULL;
		port_part.slen = 0;
	    } else {
		port_part.ptr = (char*)last_colon_pos + 1;
		port_part.slen = end - port_part.ptr;
	    }

	    /* Make sure port number is valid */
	    for (i=0; i<port_part.slen; ++i) {
		if (!pj_isdigit(port_part.ptr[i]))
		    return PJ_EINVAL;
	    }
	    port = pj_strtoul(&port_part);
	    if (port > 65535)
		return PJ_EINVAL;

	    if (p_hostpart)
		*p_hostpart = hostpart;
	    if (p_port)
		*p_port = (pj_uint16_t)port;

	    return PJ_SUCCESS;

	} else {
	    /* Treat everything as part of the IPv6 IP address */
	    if (p_hostpart)
		*p_hostpart = *str;
	    if (p_port)
		*p_port = 0;

	    return PJ_SUCCESS;
	}

    } else {
	return PJ_EAFNOTSUP;
    }

}

/*
 * Parse address
 */
PJ_DEF(pj_status_t) pj_sockaddr_parse( int af, unsigned options,
				       const pj_str_t *str,
				       pj_sockaddr *addr)
{
    pj_str_t hostpart;
    pj_uint16_t port;
    pj_status_t status;

    PJ_ASSERT_RETURN(addr, PJ_EINVAL);
    PJ_ASSERT_RETURN(af==PJ_AF_UNSPEC ||
		     af==PJ_AF_INET ||
		     af==PJ_AF_INET6, PJ_EINVAL);
    PJ_ASSERT_RETURN(options == 0, PJ_EINVAL);

    status = pj_sockaddr_parse2(af, options, str, &hostpart, &port, &af);
    if (status != PJ_SUCCESS)
	return status;
    
#if !defined(PJ_HAS_IPV6) || !PJ_HAS_IPV6
    if (af==PJ_AF_INET6)
	return PJ_EIPV6NOTSUP;
#endif

    status = pj_sockaddr_init(af, addr, &hostpart, port);
#if defined(PJ_HAS_IPV6) && PJ_HAS_IPV6
    if (status != PJ_SUCCESS && af == PJ_AF_INET6) {
	/* Parsing does not yield valid address. Try to treat the last 
	 * portion after the colon as port number.
	 */
	const char *last_colon_pos=NULL, *p;
	const char *end = str->ptr + str->slen;
	unsigned long long_port;
	pj_str_t port_part;
	int i;

	/* Parse as IPv6:port */
	for (p=str->ptr; p!=end; ++p) {
	    if (*p == ':')
		last_colon_pos = p;
	}

	if (last_colon_pos == NULL)
	    return status;

	hostpart.ptr = (char*)str->ptr;
	hostpart.slen = last_colon_pos - str->ptr;

	port_part.ptr = (char*)last_colon_pos + 1;
	port_part.slen = end - port_part.ptr;

	/* Make sure port number is valid */
	for (i=0; i<port_part.slen; ++i) {
	    if (!pj_isdigit(port_part.ptr[i]))
		return status;
	}
	long_port = pj_strtoul(&port_part);
	if (long_port > 65535)
	    return status;

	port = (pj_uint16_t)long_port;

	status = pj_sockaddr_init(PJ_AF_INET6, addr, &hostpart, port);
    }
#endif
    
    return status;
}

/*
 * Tools to get address string.
 */
PJ_DEF(const char *)pj_addr_string(const pj_sockaddr_t *addr)
{
    static char str[PJ_INET6_ADDRSTRLEN];

    pj_inet_ntop(((const pj_sockaddr*)addr)->addr.sa_family, 
		 pj_sockaddr_get_addr(addr),
		 str, sizeof(str));
    return str;
}

/* Resolve the IP address of local machine */
PJ_DEF(pj_status_t) pj_gethostip(int af, pj_sockaddr *addr)
{
	int rc = PJ_SUCCESS;
	char * host_name = "localhost";

	if (af == AF_INET)
	{
		tcpip_adapter_ip_info_t ip;
		rc = tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);
		memcpy(&addr->ipv4.sin_addr, &ip.ip, sizeof(ip4_addr_t));

		addr->addr.sa_family = PJ_AF_INET;
	}
	else
	{
		rc = mdns_query_aaaa(host_name, 2000, &addr->ipv6.sin6_addr);
		addr->addr.sa_family = PJ_AF_INET6;
	}

	return rc;
}

/* Get IP interface for sending to the specified destination */
PJ_DEF(pj_status_t) pj_getipinterface(int af,
                                      const pj_str_t *dst,
                                      pj_sockaddr *itf_addr,
                                      pj_bool_t allow_resolve,
                                      pj_sockaddr *p_dst_addr)
{
    pj_sockaddr dst_addr;
    pj_sock_t fd;
    int len;
    pj_uint8_t zero[64];
    pj_status_t status;

    pj_sockaddr_init(af, &dst_addr, NULL, 53);
    status = pj_inet_pton(af, dst, pj_sockaddr_get_addr(&dst_addr));
    if (status != PJ_SUCCESS) {
	/* "dst" is not an IP address. */
	if (allow_resolve) {
	    status = pj_sockaddr_init(af, &dst_addr, dst, 53);
	} else {
	    pj_str_t cp;

	    if (af == PJ_AF_INET) {
		cp = pj_str("1.1.1.1");
	    } else {
		cp = pj_str("1::1");
	    }
	    status = pj_sockaddr_init(af, &dst_addr, &cp, 53);
	}

	if (status != PJ_SUCCESS)
	    return status;
    }

    /* Create UDP socket and connect() to the destination IP */
    status = pj_sock_socket(af, pj_SOCK_DGRAM(), 0, &fd);
    if (status != PJ_SUCCESS) {
	return status;
    }

    status = pj_sock_connect(fd, &dst_addr, pj_sockaddr_get_len(&dst_addr));
    if (status != PJ_SUCCESS) {
	pj_sock_close(fd);
	return status;
    }

    len = sizeof(*itf_addr);
    status = pj_sock_getsockname(fd, itf_addr, &len);
    if (status != PJ_SUCCESS) {
	pj_sock_close(fd);
	return status;
    }

    pj_sock_close(fd);

    /* Check that the address returned is not zero */
    pj_bzero(zero, sizeof(zero));
    if (pj_memcmp(pj_sockaddr_get_addr(itf_addr), zero,
		  pj_sockaddr_get_addr_len(itf_addr))==0)
    {
	return PJ_ENOTFOUND;
    }

    if (p_dst_addr)
	*p_dst_addr = dst_addr;

    return PJ_SUCCESS;
}

/* Get the default IP interface */
PJ_DEF(pj_status_t) pj_getdefaultipinterface(int af, pj_sockaddr *addr)
{
    pj_str_t cp;

    if (af == PJ_AF_INET) {
	cp = pj_str("1.1.1.1");
    } else {
	cp = pj_str("1::1");
    }

    return pj_getipinterface(af, &cp, addr, PJ_FALSE, NULL);
}


/*
 * Bind socket at random port.
 */
PJ_DEF(pj_status_t) pj_sock_bind_random(  pj_sock_t sockfd,
				          const pj_sockaddr_t *addr,
				          pj_uint16_t port_range,
				          pj_uint16_t max_try)
{
    pj_sockaddr bind_addr;
    int addr_len;
    pj_uint16_t base_port;
    pj_status_t status = PJ_SUCCESS;

    PJ_CHECK_STACK();

    PJ_ASSERT_RETURN(addr, PJ_EINVAL);

    pj_sockaddr_cp(&bind_addr, addr);
    addr_len = pj_sockaddr_get_len(addr);
    base_port = pj_sockaddr_get_port(addr);

    if (base_port == 0 || port_range == 0) {
	return pj_sock_bind(sockfd, &bind_addr, addr_len);
    }

    for (; max_try; --max_try) {
	pj_uint16_t port;
	port = (pj_uint16_t)(base_port + pj_rand() % (port_range + 1));
	pj_sockaddr_set_port(&bind_addr, port);
	status = pj_sock_bind(sockfd, &bind_addr, addr_len);
	if (status == PJ_SUCCESS)
	    break;
    }

    return status;
}


/*
 * Adjust socket send/receive buffer size.
 */
PJ_DEF(pj_status_t) pj_sock_setsockopt_sobuf( pj_sock_t sockfd,
					      pj_uint16_t optname,
					      pj_bool_t auto_retry,
					      unsigned *buf_size)
{
    pj_status_t status;
    int try_size, cur_size, i, step, size_len;
    enum { MAX_TRY = 20 };

    PJ_CHECK_STACK();

    PJ_ASSERT_RETURN(sockfd != PJ_INVALID_SOCKET &&
		     buf_size &&
		     *buf_size > 0 &&
		     (optname == pj_SO_RCVBUF() ||
		      optname == pj_SO_SNDBUF()),
		     PJ_EINVAL);

    size_len = sizeof(cur_size);
    status = pj_sock_getsockopt(sockfd, pj_SOL_SOCKET(), optname,
				&cur_size, &size_len);
    if (status != PJ_SUCCESS)
	return status;

    try_size = *buf_size;
    step = (try_size - cur_size) / MAX_TRY;
    if (step < 4096)
	step = 4096;

    for (i = 0; i < (MAX_TRY-1); ++i) {
	if (try_size <= cur_size) {
	    /* Done, return current size */
	    *buf_size = cur_size;
	    break;
	}

	status = pj_sock_setsockopt(sockfd, pj_SOL_SOCKET(), optname,
				    &try_size, sizeof(try_size));
	if (status == PJ_SUCCESS) {
	    status = pj_sock_getsockopt(sockfd, pj_SOL_SOCKET(), optname,
					&cur_size, &size_len);
	    if (status != PJ_SUCCESS) {
		/* Ops! No info about current size, just return last try size
		 * and quit.
		 */
		*buf_size = try_size;
		break;
	    }
	}

	if (!auto_retry)
	    break;

	try_size -= step;
    }

    return status;
}


PJ_DEF(char *) pj_addr_str_print( const pj_str_t *host_str, int port, 
				  char *buf, int size, unsigned flag)
{
    enum {
	WITH_PORT = 1
    };
    char *bquote, *equote;
    int af = pj_AF_UNSPEC();    
    pj_in6_addr dummy6;

    /* Check if this is an IPv6 address */
    if (pj_inet_pton(pj_AF_INET6(), host_str, &dummy6) == PJ_SUCCESS)
	af = pj_AF_INET6();

    if (af == pj_AF_INET6()) {
	bquote = "[";
	equote = "]";    
    } else {
	bquote = "";
	equote = "";    
    } 

    if (flag & WITH_PORT) {
	pj_ansi_snprintf(buf, size, "%s%.*s%s:%d",
			 bquote, (int)host_str->slen, host_str->ptr, equote, 
			 port);
    } else {
	pj_ansi_snprintf(buf, size, "%s%.*s%s",
			 bquote, (int)host_str->slen, host_str->ptr, equote);
    }
    return buf;
}