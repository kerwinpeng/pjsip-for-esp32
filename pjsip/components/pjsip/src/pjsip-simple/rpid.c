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
#include <pjsip-simple/rpid.h>
#include <pjsip-simple/errno.h>
#include <pj/pj_assert.h>
#include <pj/guid.h>
#include <pj/pool.h>
#include <pj/pj_string.h>
#include <pjlib-util/xml.h>

static const pj_str_t DM_NAME = {"xmlns:dm", 8};
static const pj_str_t DM_VAL = {"urn:ietf:params:xml:ns:pidf:data-model", 38};
static const pj_str_t RPID_NAME = {"xmlns:rpid", 10};
static const pj_str_t RPID_VAL = {"urn:ietf:params:xml:ns:pidf:rpid", 32};

static const pj_str_t DM_NOTE = {"dm:note", 7};
static const pj_str_t DM_PERSON = {"dm:person", 9};
static const pj_str_t ID = {"id", 2};
static const pj_str_t NOTE = {"note", 4};
static const pj_str_t RPID_ACTIVITIES = {"rpid:activities", 15};

/* Duplicate RPID element */
PJ_DEF(void) pjrpid_element_dup(pj_pool_t *pool, pjrpid_element *dst,
				const pjrpid_element *src)
{
    pj_memcpy(dst, src, sizeof(pjrpid_element));
    pj_strdup(pool, &dst->id, &src->id);
    pj_strdup(pool, &dst->note, &src->note);
}

/* Comparison function to find node name substring */
static pj_bool_t substring_match(const pj_xml_node *node, 
				 const char *part_name,
				 pj_ssize_t part_len)
{
    pj_str_t end_name;

    if (part_len < 1)
	part_len = pj_ansi_strlen(part_name);

    if (node->name.slen < part_len)
	return PJ_FALSE;

    end_name.ptr = node->name.ptr + (node->name.slen - part_len);
    end_name.slen = part_len;

    return pj_strnicmp2(&end_name, part_name, part_len)==0;
}

/* Util to find child node with the specified substring */
static pj_xml_node *find_node(const pj_xml_node *parent, 
			      const char *part_name)
{
    const pj_xml_node *node = parent->node_head.next, 
		      *head = (pj_xml_node*) &parent->node_head;
    pj_ssize_t part_len = pj_ansi_strlen(part_name);

    while (node != head) {
	if (substring_match(node, part_name, part_len))
	    return (pj_xml_node*) node;

	node = node->next;
    }

    return NULL;
}