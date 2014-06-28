/*
 * Copyright (c) 2002-2004, Adam Dunkels.
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
 * This file is part of the Contiki operating system
 *
 * $Id: main.c,v 1.1 2006/04/17 15:15:44 kthacker Exp $
 *
 */

#include "contiki.h"

#include "ctk.h"
#include "ctk-draw.h"
#include "ek.h"
#include "log.h"
#include "program-handler.h"

#include "uip.h"
#include "uip_arp.h"

#include "resolv.h"

#include "configedit-dsc.h"
#include "directory-dsc.h"
#include "processes-dsc.h"

//#include "cfs-cbm.h"
#include "cfs-cpc.h"

#include "clock.h"

#include "arch.h"

unsigned char
uip_fw_forward(void)
{
  return 0;
}
void
uip_fw_periodic(void)
{
  return;
}
/*-----------------------------------------------------------------------------------*/
EK_EVENTHANDLER(eventhandler, ev, data)
{
  switch(ev) {
  case EK_EVENT_INIT:
    program_handler_load("conio.prg",NULL);
//    program_handler_load("config.prg", NULL);
    break;
  }
}
/*-----------------------------------------------------------------------------------*/
const EK_PROCESS(init, "Init", EK_PRIO_LOWEST,
	   eventhandler, NULL, NULL);
/*-----------------------------------------------------------------------------------*/
#if LOG_CONF_ENABLED
void
log_message(char *part1, char *part2)
{
  while(*part1 != 0) {
    putchar(*part1++);
  }

  while(*part2 != 0) {
    putchar(*part2++);
  }
  
  putchar('\r');
  putchar('\n');
}
#endif
/*-----------------------------------------------------------------------------------*/
void
main(void)
{
  
  log_message("Starting ", CONTIKI_VERSION_STRING);
  
  ek_init();

  ek_start(&init);
    
 // log_message(": TCP/IP", "");
    
 // tcpip_init(NULL);

 // resolv_init(NULL); 

  log_message(": CTK GUI", "");
  ctk_init();

  log_message(": Initial filesystem", "");
  cfs_cpc_init(NULL);

  program_handler_init();
   
  program_handler_add(&directory_dsc, "Directory", 1);
//  program_handler_add(&configedit_dsc, "Configuration", 1);
  program_handler_add(&processes_dsc, "Processes", 1);  
  
  
  log_message("Starting process scheduling", "");  

  while(1) {
    if(ek_run() == 0) {
      program_handler_load("welcome.prg", NULL);
      break;
    }
  }
    
  while(1) {
    ek_run();
  }
}
/*-----------------------------------------------------------------------------------*/
void
reset(void)
{
/*  asm("lda #$36");
  asm("sta $01");
  asm("jmp $fce2");
*/
}

