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
 * This file is part of the Contiki desktop environment
 *
 * $Id: wget.c,v 1.1 2003/07/30 23:10:49 adamdunkels Exp $
 *
 */


#include "ctk.h"
#include "dispatcher.h"
#include "webclient.h"
#include "resolv.h"
#include "petsciiconv.h"
#include "uip_main.h"
#include "loader.h"

#include <c64.h>
#include <cbm.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>


static struct ctk_window window;

static struct ctk_label hostlabel =
  {CTK_LABEL(0, 1, 7, 1, "Server:")};
static char hostedit[40];
static char host[40];
static struct ctk_textentry hosttextentry =
  {CTK_TEXTENTRY(8, 1, 26, 1, hostedit, 38)};

static struct ctk_label filelabel =
  {CTK_LABEL(0, 3, 5, 1, "File:")};
static char fileedit[40];
static char file[40];
static struct ctk_textentry filetextentry =
  {CTK_TEXTENTRY(8, 3, 26, 1, fileedit, 38)};

static struct ctk_label savefilenamelabel =
  {CTK_LABEL(0, 5, 14, 1, "Save filename:")};
static char savefilename[40];
static struct ctk_textentry savefilenametextentry =
  {CTK_TEXTENTRY(15, 5, 19, 1, savefilename, 38)};

static struct ctk_button filebutton =
  {CTK_BUTTON(0, 7, 13, "Download file")};

static struct ctk_button d64button =
  {CTK_BUTTON(17, 7, 18, "Download D64 disk")};

static struct ctk_label statustext =
  {CTK_LABEL(0, 9, 36, 1, "")};
static char statusmsg[40];

static struct ctk_window d64dialog;
static struct ctk_label overwritelabel =
  {CTK_LABEL(0, 1, 36, 1, "This will overwrite the entire disk!")};
static struct ctk_label makesurelabel1 =
  {CTK_LABEL(7, 3, 22, 1, "Make sure you have the")};
static struct ctk_label makesurelabel2 =
  {CTK_LABEL(6, 4, 24, 1, "right disk in the drive!")};
static struct ctk_button overwritebutton =
  {CTK_BUTTON(2, 6, 14, "Overwrite disk")};
static struct ctk_button cancelbutton =
  {CTK_BUTTON(26, 6, 6, "Cancel")};

static DISPATCHER_SIGHANDLER(wget_sighandler, s, data);
static struct dispatcher_proc p =
  {DISPATCHER_PROC("Web downloader", NULL, wget_sighandler, webclient_appcall)};
static ek_id_t id;

/* State */

#define DLOAD_NONE 0
#define DLOAD_FILE 1
#define DLOAD_D64  2
static u8_t dload_state;
static unsigned long dload_bytes;



struct drv_state {
  u8_t track;
  u8_t sect;
};

static struct drv_state ds;

static char buffer[256];
static u16_t bufferptr;

/*-----------------------------------------------------------------------------------*/
/* wget_init();
 *
 * Initializes and starts the web browser. Called either at startup or
 * to open the browser window.
 */
LOADER_INIT_FUNC(wget_init)
{
  if(id == EK_ID_NONE) {
    id = dispatcher_start(&p);
    
    /* Create the main window. */
    ctk_window_new(&window, 36, 10, "Web downloader");

    CTK_WIDGET_ADD(&window, &hostlabel);
    CTK_WIDGET_ADD(&window, &hosttextentry);

    CTK_WIDGET_ADD(&window, &filelabel);
    CTK_WIDGET_ADD(&window, &filetextentry);
    
    CTK_WIDGET_ADD(&window, &savefilenamelabel);
    CTK_WIDGET_ADD(&window, &savefilenametextentry);

    CTK_WIDGET_ADD(&window, &filebutton);

    CTK_WIDGET_ADD(&window, &d64button);

    CTK_WIDGET_ADD(&window, &statustext);

    memset(hostedit, 0, sizeof(hostedit));
    memset(fileedit, 0, sizeof(fileedit));
    memset(savefilename, 0, sizeof(savefilename));

    ctk_dialog_new(&d64dialog, 36, 8);
    CTK_WIDGET_ADD(&d64dialog, &overwritelabel);
    CTK_WIDGET_ADD(&d64dialog, &makesurelabel1);
    CTK_WIDGET_ADD(&d64dialog, &makesurelabel2);
    CTK_WIDGET_ADD(&d64dialog, &overwritebutton);
    CTK_WIDGET_ADD(&d64dialog, &cancelbutton);
    
    
    /* Attach as a listener to a number of signals ("Button activate",
       "Hyperlink activate" and "Hyperlink hover", and the resolver's
       signal. */
    dispatcher_listen(ctk_signal_window_close);
    dispatcher_listen(ctk_signal_button_activate);
    dispatcher_listen(resolv_signal_found);
  }
  ctk_window_open(&window);
}
/*-----------------------------------------------------------------------------------*/
static void
show_statustext(char *text)
{
  ctk_label_set_text(&statustext, text);
  CTK_WIDGET_REDRAW(&statustext);
}
/*-----------------------------------------------------------------------------------*/
/* open_url():
 *
 * Called when the URL present in the global "url" variable should be
 * opened. It will call the hostname resolver as well as the HTTP
 * client requester.
 */
static void
start_get(void)
{
  u16_t addr[2];
  
  /* First check if the host is an IP address. */
  if(uip_main_ipaddrconv(host, (unsigned char *)addr) == 0) {    
    
    /* Try to lookup the hostname. If it fails, we initiate a hostname
       lookup and print out an informative message on the statusbar. */
    if(resolv_lookup(host) == NULL) {
      resolv_query(host);
      show_statustext("Resolving host...");
      return;
    }
  }

  /* The hostname we present in the hostname table, so we send out the
     initial GET request. */
  if(webclient_get(host, 80, file) == 0) {
    show_statustext("Out of memory error.");
  } else {
    show_statustext("Connecting...");
  }
}
/*-----------------------------------------------------------------------------------*/
static
DISPATCHER_SIGHANDLER(wget_sighandler, s, data)
{
  int ret;
  static unsigned char i;
  DISPATCHER_SIGHANDLER_ARGS(s, data);

  if(s == ctk_signal_button_activate) {
    if(data == (void *)&filebutton) {
      ret = cbm_open(2, 8, 2, savefilename);
      if(ret == -1) {
	sprintf(statusmsg, "Open error with '%s'", savefilename);
	show_statustext(statusmsg);
      } else {
	strncpy(host, hostedit, sizeof(host));
	strncpy(file, fileedit, sizeof(file));
	petsciiconv_toascii(host, sizeof(host));
	petsciiconv_toascii(file, sizeof(file));
	start_get();
	dload_bytes = 0;
	dload_state = DLOAD_FILE;
      }
    } else if(data == (void *)&d64button) {
      ctk_dialog_open(&d64dialog);
    } else if(data == (void *)&cancelbutton) {
      ctk_dialog_close();
    } else if(data == (void *)&overwritebutton) {
      ctk_dialog_close();
      strncpy(host, hostedit, sizeof(host));
      strncpy(file, fileedit, sizeof(file));
      petsciiconv_toascii(host, sizeof(host));
      petsciiconv_toascii(file, sizeof(file));
      start_get();
      dload_bytes = 0;
      dload_state = DLOAD_D64;
      ds.track = 1;
      ds.sect = 0;
      bufferptr = 0;
    }
  } else if(s == resolv_signal_found) {
    /* Either found a hostname, or not. */
    if((char *)data != NULL &&
       resolv_lookup((char *)data) != NULL) {
      start_get();
    } else {
      show_statustext("Host not found.");
    }
  } else if(s == ctk_signal_window_close) {
    dispatcher_exit(&p);
    id = EK_ID_NONE;
    LOADER_UNLOAD();
  }
}
/*-----------------------------------------------------------------------------------*/
/* webclient_aborted():
 *
 * Callback function. Called from the webclient when the HTTP
 * connection was abruptly aborted.
 */
void
webclient_aborted(void)
{
  show_statustext("Connection reset by peer");
}
/*-----------------------------------------------------------------------------------*/
/* webclient_timedout():
 *
 * Callback function. Called from the webclient when the HTTP
 * connection timed out.
 */
void
webclient_timedout(void)
{
  show_statustext("Connection timed out");
  if(dload_state == DLOAD_FILE) {
    cbm_close(2);
  }

}
/*-----------------------------------------------------------------------------------*/
/* webclient_closed():
 *
 * Callback function. Called from the webclient when the HTTP
 * connection was closed after a request from the "webclient_close()"
 * function. .
 */
void
webclient_closed(void)
{  
  show_statustext("Done.");
}
/*-----------------------------------------------------------------------------------*/
/* webclient_closed():
 *
 * Callback function. Called from the webclient when the HTTP
 * connection is connected.
 */
void
webclient_connected(void)
{    
  show_statustext("Request sent...");
}
/*-----------------------------------------------------------------------------------*/
static void
x_open(u8_t f, u8_t d, u8_t cmd, u8_t *fname)
{
  u8_t ret;
  
  ret = cbm_open(f, d, cmd, fname);
  if(ret != 0) {
    /*    printf("open: error %d\n", ret);*/
    /*    ctk_label_set_text(&statuslabel, "Open err");
	  CTK_WIDGET_REDRAW(&statuslabel);*/
    show_statustext("Open error");
  }
  
}

#if 0
static u8_t cmd[32];
static void
read_sector(u8_t device, u8_t track, u8_t sect, void *mem)
{  
  int ret;
  
  x_open(15, device, 15, NULL);
  x_open(2, device, 2, "#");

  /*  sprintf(cmd, "u1: 2  0%3d%3d", track, sect);  */
  strcpy(cmd, "u1: 2  0");
  cmd[8] = ' ';
  cmd[9] = '0' + track / 10;
  cmd[10] = '0' + track % 10;
  cmd[11] = ' ';
  cmd[12] = '0' + sect / 10;
  cmd[13] = '0' + sect % 10;
  cmd[14] = 0;
  cbm_write(15, cmd, strlen(cmd));
  /*  printf("%s\n", cmd);*/
    
  ret = cbm_read(2, mem, 256);
  if(ret == -1) {
    ctk_label_set_text(&statuslabel, "Read err");
    CTK_WIDGET_REDRAW(&statuslabel);
  }
  /*  printf("read: read %d bytes\n", ret);*/

  cbm_close(2);
  cbm_close(15);

}
#endif /* 0 */

static void
write_sector(u8_t device, u8_t track, u8_t sect, void *mem)
{
  u16_t ret;
  u8_t cmd[32];
  
  x_open(15, device, 15, NULL);
  x_open(2, device, 2, "#");

  sprintf(cmd, "u2: 2  0%3d%3d", track, sect);  
  cbm_write(15, cmd, strlen(cmd));
  printf("%s\n", cmd);
    
  ret = cbm_write(2, mem, 256);
  /*  ret = 0;*/
  if(ret == -1) {
    sprintf(statusmsg, "Write error at %d:%d", track, sect);
    show_statustext(statusmsg);
  } else {
    sprintf(statusmsg, "Wrote %d:%d", track, sect);
    show_statustext(statusmsg);
  }
  /*  printf("write: wrote %d bytes\n", ret);*/

  cbm_close(2);
  cbm_close(15);
}

static u8_t
next_sector(void)
{
  ++ds.sect;
  if(ds.track < 18) {
    if(ds.sect == 21) {
      ++ds.track;
      ds.sect = 0;
    }
  } else if(ds.track < 25) {
    if(ds.sect == 19) {
      ++ds.track;
      ds.sect = 0;
    }
  } else if(ds.track < 31) {
    if(ds.sect == 18) {
      ++ds.track;
      ds.sect = 0;
    }
  } else if(ds.track < 36) {
    if(ds.sect == 17) {
      ++ds.track;
      ds.sect = 0;
    }
  }

  if(ds.track == 36) {
    return 1;
  }
  return 0;
}
/*-----------------------------------------------------------------------------------*/
static void
write_buffer(void)
{
  write_sector(8, ds.track, ds.sect, buffer);
  if(next_sector() != 0) {
    dload_state = DLOAD_NONE;
  }
}
static void
handle_d64_data(char *data, u16_t len)
{
  u16_t bufferlen;

  while(dload_state == DLOAD_D64 &&
	len > 0) {
    bufferlen = sizeof(buffer) - bufferptr;
    if(len < bufferlen) {
      bufferlen = len;
    }
    
    memcpy(&buffer[bufferptr], data, bufferlen);

    data += bufferlen;
    bufferptr += bufferlen;
    len -= bufferlen;
    
    if(bufferptr == sizeof(buffer)) {
      write_buffer();
      bufferptr = 0;
    }
  }
}
/*-----------------------------------------------------------------------------------*/
/* webclient_datahandler():   
 *
 * Callback function. Called from the webclient module when HTTP data
 * has arrived.
 */
void
webclient_datahandler(char *data, u16_t len)
{
  int ret;
  
  if(len > 0) {
    dload_bytes += len;    
    sprintf(statusmsg, "Downloading (%lu bytes)", dload_bytes);
    show_statustext(statusmsg);
    if(dload_state == DLOAD_D64) {
      handle_d64_data(data, len);
    } else if(dload_state == DLOAD_FILE) {      
      ret = cbm_write(2, data, len);       
      if(ret != len) {
	sprintf(statusmsg, "Wrote only %d bytes", ret);
	  show_statustext(statusmsg);	  
      }
    }
  }
  
  if(data == NULL) {
    if(dload_state == DLOAD_FILE) {
      cbm_close(2);
    }
    dload_state = DLOAD_NONE;
    sprintf(statusmsg, "Finished downloading %lu bytes", dload_bytes);
    show_statustext(statusmsg);
  }
}
/*-----------------------------------------------------------------------------------*/
