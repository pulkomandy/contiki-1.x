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
 * 3. The name of the author may not be used to endorse or promote
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
 * This file is part of the Contiki VNC client.
 *
 * $Id: vnc-draw.h,v 1.2 2004/06/06 06:03:03 adamdunkels Exp $
 *
 */

#ifndef __VNC_DRAW_H__
#define __VNC_DRAW_H__

#include "uip_arch.h"

/* Pointer to the bitmap area in memory. */
extern u8_t vnc_draw_bitmap[];

/* Initialize the vnc-draw module. */
void vnc_draw_init(void);

/* Draw one line of pixels starting at point (x, y). The pixel data is
   given by the "data" argument and the length of data is given by the
   "datalen" argument. The data format is one pixel per byte in bgr233
   format (bbgggrrr). */
void vnc_draw_pixelline(u16_t x, u16_t y,
			u8_t *data, u16_t datalen);

/* The following functions should return the x and y coordinates and
   the width and height of the viewport. */
u16_t vnc_draw_viewport_x(void);
u16_t vnc_draw_viewport_y(void);
u16_t vnc_draw_viewport_w(void);
u16_t vnc_draw_viewport_h(void);

#endif /* __VNC_DRAW_H__ */
