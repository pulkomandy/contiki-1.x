/*
 * Copyright (c) 2003, Adam Dunkels.
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
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
 * This file is part of the Contiki desktop OS.
 *
 * $Id: shell.c,v 1.1 2006/04/17 15:18:20 kthacker Exp $
 *
 */

#include "program-handler.h"
#include "loader.h"
#include "cfs.h"
#include "uip.h"
#include "uip_arp.h"
#include "resolv.h"

#include "shell.h"

#include <string.h>

static char showingdir = 0;
static struct cfs_dir dir;
static unsigned int totsize;

struct ptentry {
  char c;
  void (* pfunc)(char *str);
};

/*-----------------------------------------------------------------------------------*/
static void
parse(CC_REGISTER_ARG char *str, struct ptentry *t)
{
  CC_REGISTER_ARG struct ptentry *p;
  char *sstr;

  sstr = str;
  
  /* Loop over the parse table entries in t in order to find one that
     matches the first character in str. */
  for(p = t; p->c != 0; ++p) {
    if(*str == p->c) {
      /* Skip rest of the characters up to the first space. */
      while(*str != ' ') {
	++str;
      }

      /* Skip all spaces.*/
      while(*str == ' ') {
	++str;
      }

      /* Call parse table entry function and return. */
      p->pfunc(str);
      return;
    }
  }

  /* Did not find matching entry in parse table. We just call the
     default handler supplied by the caller and return. */
  p->pfunc(str);
}
/*-----------------------------------------------------------------------------------*/
static void
inttostr(CC_REGISTER_ARG char *str, unsigned int i)
{
  str[0] = '0' + i / 100;
  if(str[0] == '0') {
    str[0] = ' ';
  }
  str[1] = '0' + (i / 10) % 10;
  if(str[0] == ' ' && str[1] == '0') {
    str[1] = ' ';
  }
  str[2] = '0' + i % 10;
  str[3] = ' ';
  str[4] = 0;
}
/*-----------------------------------------------------------------------------------*/
static void
processes(char *str)
{
  static char idstr[5];
  struct ek_proc *p;

  shell_output("Processes:", "");
  /* Step through each possible process ID and see if there is a
     matching process. */
  for(p = EK_PROCS(); p != NULL; p = p->next) {
    inttostr(idstr, p->id);
    shell_output(idstr, (char *)p->name);
  }
}
/*-----------------------------------------------------------------------------------*/
static char *
nullterminate(char *str)
{
  char *nt;

  /* Nullterminate string. Start with finding newline character. */
  for(nt = str; *nt != '\r' &&
	*nt != '\n'; ++nt);

  /* Replace newline with a null char. */
  *nt = 0;

  /* Remove trailing spaces. */
  while(nt > str && *(nt - 1) == ' ') {
    *(nt - 1) = 0;
    --nt;
  }
  
  /* Return pointer to null char. */
  return nt;
}
/*-----------------------------------------------------------------------------------*/
static void
runfile(char *str)
{
  nullterminate(str);

  if(strlen(str) > 0) {
    /* Call loader function. */
    program_handler_load(str, NULL);
    shell_output("Starting program ", str);  
  } else {
    shell_output("Must supply a program name", "");  
  }
}
/*-----------------------------------------------------------------------------------*/
static void
execfile(char *str)
{
  runfile(str);
  shell_quit(NULL);
}
/*-----------------------------------------------------------------------------------*/
static void
killproc(char *str)
{
  char procnum, j, c;
  char procstr[5];

  nullterminate(str);
  
  procnum = 0;
  
  for(j = 0; j < 4; ++j) {
    c = str[j];
    if(c >= '0' && c <= '9') {
      procnum = procnum * 10 + (str[j] - '0');
    } else {
      break;
    }
  }
  if(procnum != 0) {
    inttostr(procstr, procnum);
    shell_output("Killing process ", procstr);
    ek_post(procnum, EK_EVENT_REQUEST_EXIT, NULL);
  } else {
    shell_output("Invalid process number", "");
  }
  
}
/*-----------------------------------------------------------------------------------*/
static void
help(char *str)
{
  shell_output("Available commands:", "");
  shell_output("run  - start program", "");
  shell_output("exec - start program & exit shell", "");  
  shell_output("ps   - show processes", "");
  shell_output("kill - kill process", "");
  shell_output("ls   - display directory", "");
  shell_output("quit - quit shell", "");
  shell_output("?    - show this help", "");      
}
/*-----------------------------------------------------------------------------------*/
static void
directory(char *str)
{
  if(cfs_opendir(&dir, "/") != 0) {
    shell_output("Cannot open directory", "");
    showingdir = 0;
  } else {
    shell_output("Disk directory:", "");
    showingdir = 1;
    totsize = 0;
    ek_post(EK_PROC_ID(EK_CURRENT()), EK_EVENT_CONTINUE, NULL);
  }
  
}
/*-----------------------------------------------------------------------------------*/
static void
none(char *str)
{
}
/*-----------------------------------------------------------------------------------*/
static struct ptentry configparsetab[] =
  {{'e', execfile},
   {'r', runfile},
   {'k', killproc},   
   {'p', processes},
   {'l', directory},
   {'q', shell_quit},
   {'?', help},

   /* Default action */
   {0, none}};
/*-----------------------------------------------------------------------------------*/
void
shell_init(void)
{
}
/*-----------------------------------------------------------------------------------*/
void
shell_start(void)
{
  showingdir = 0;
  shell_output("Contiki command shell", "");
  shell_output("Type '?' and return for help", "");  
  shell_prompt("contiki-c64> "); 
}
/*-----------------------------------------------------------------------------------*/
void
shell_input(char *cmd)
{
  if(showingdir != 0) {
    showingdir = 0;
    shell_output("Directory stopped", "");
    cfs_closedir(&dir);
  }
  parse(cmd, configparsetab);
  if(showingdir == 0) {
    shell_prompt("contiki-c64> ");
  }
}
/*-----------------------------------------------------------------------------------*/
void
shell_eventhandler(ek_event_t ev, ek_data_t data)
{
  static struct cfs_dirent dirent;
  static char size[10];

  if(ev == EK_EVENT_CONTINUE) {
    if(showingdir != 0) {
      if(cfs_readdir(&dir, &dirent) != 0) {
	cfs_closedir(&dir);
	showingdir = 0;
	inttostr(size, totsize);
	shell_output("Total number of blocks:", size);
	shell_prompt("contiki-c64> ");
      } else {
		  dirent.size >>= 10;
	totsize += dirent.size;
	inttostr(size, dirent.size);
	shell_output(size, dirent.name);
	ek_post(EK_PROC_ID(EK_CURRENT()), EK_EVENT_CONTINUE, NULL);
      }
    }
  }
}
/*-----------------------------------------------------------------------------------*/
