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
#include <pj/except.h>
#include <pj/pj_os.h>
#include <pj/pj_assert.h>
#include <pj/log.h>
#include <pj/pj_errno.h>
#include <pj/pj_string.h>

static long thread_local_id = -1;

#if !defined(PJ_EXCEPTION_USE_WIN32_SEH) || PJ_EXCEPTION_USE_WIN32_SEH==0
PJ_DEF(void) pj_throw_exception_(int exception_id)
{
    struct pj_exception_state_t *handler;

    handler = (struct pj_exception_state_t*) 
	      pj_thread_local_get(thread_local_id);
    if (handler == NULL) {
        PJ_LOG(1,("except.c", "!!!FATAL: unhandled exception %d!\n", exception_id));

        pj_assert(handler != NULL);
        /* This will crash the system! */
    }
    pj_pop_exception_handler_(handler);
    pj_longjmp(handler->state, exception_id);
}

static void exception_cleanup(void)
{
    if (thread_local_id != -1) {
        pj_thread_local_free(thread_local_id);
        thread_local_id = -1;
    }
}

PJ_DEF(void) pj_push_exception_handler_(struct pj_exception_state_t *rec)
{
    struct pj_exception_state_t *parent_handler = NULL;

    if (thread_local_id == -1) {
	pj_thread_local_alloc(&thread_local_id);

	pj_assert(thread_local_id != -1);
	pj_atexit(&exception_cleanup);
    }
    parent_handler = (struct pj_exception_state_t *)
		      pj_thread_local_get(thread_local_id);

    rec->prev = parent_handler;
    pj_thread_local_set(thread_local_id, rec);
}

PJ_DEF(void) pj_pop_exception_handler_(struct pj_exception_state_t *rec)
{
    struct pj_exception_state_t *handler;

    handler = (struct pj_exception_state_t *)
	      pj_thread_local_get(thread_local_id);
    if (handler && handler==rec) {
	pj_thread_local_set(thread_local_id, handler->prev);
    }
}
#endif



