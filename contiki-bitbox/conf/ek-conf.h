/*
 * Copyright (c) 2002, Adam Dunkels.
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution. 
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgement:
 *        This product includes software developed by Adam Dunkels. 
 * 4. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.  
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
 *
 * This file is part of the "ek" event kernel.
 *
 * $Id: ek-conf.h,v 1.1 2003/05/19 09:30:09 gpz Exp $
 *
 */


#ifndef __EK_CONF_H__
#define __EK_CONF_H__

#include <time.h>

typedef void *ek_data_t;

typedef unsigned char ek_signal_t;
typedef unsigned char ek_event_t;
typedef unsigned char ek_id_t;

/* ek_ticks_t: should be defined to be the largest type that fits the
   highest timeout value used by the system. For example, if all
   timeouts are between 1 and 150, the ek_ticks_t can be typedef'd as
   "unsigned char", but if the maximum timeout is over 256, "unsigned
   short" is a better choise. */
typedef unsigned short ek_ticks_t;

/* ek_clock_t: should be defined to be the native clock ticks type
   used by the underlying system. (Look for time_t or similar.) */
typedef unsigned long ek_clock_t; 

#define EK_CONF_NUMLISTENERS  32    /* Must be 2^n */
typedef unsigned char ek_num_listeners_t;

#define EK_CONF_MAXPROCS 32

#define EK_CONF_NUMEVENTS 32
typedef unsigned char ek_num_events_t;

#endif /* __EK_CONF_H__ */
