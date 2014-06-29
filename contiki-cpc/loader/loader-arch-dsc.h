/*
 * Copyright (c) 2003, Adam Dunkels.
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
 * This file is part of the Contiki desktop OS
 *
 * $Id: loader-arch.h,v 1.1 2006/04/17 15:02:35 kthacker Exp $
 *
 */
#ifndef __LOADER_ARCH_H__
#define __LOADER_ARCH_H__

#include "dsc.h"

unsigned char loader_arch_load(const char *name, char *arg);
struct dsc *loader_arch_load_dsc(const char *name);
void loader_arch_free(void *addr);
void loader_arch_free_dsc(struct dsc *);

extern void *loader_arch_loadaddr;

#define LOADER_LOAD_DSC(name) loader_arch_load_dsc(name)
#define LOADER_LOAD(name, arg) loader_arch_load(name, arg)
#define LOADER_UNLOAD() loader_arch_free(&loader_arch_loadaddr)
#define LOADER_UNLOAD_DSC(dsc) loader_arch_free_dsc(dsc)

#endif /* __LOADER_ARCH_H__ */
