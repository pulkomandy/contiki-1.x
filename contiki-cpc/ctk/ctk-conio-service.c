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
 * This file is part of the "ctk" console GUI toolkit for cc65
 *
 * $Id: ctk-conio-service.c,v 1.12 2006/05/28 20:38:19 oliverschmidt Exp $
 *
 */

#include <conio.h>

#include "ctk.h"
#include "ctk-draw.h"

#include "ctk-draw-service.h"

#include "ctk-conio-conf.h"
#include <string.h>

#ifndef NULL
#define NULL (void *)0
#endif /* NULL */

static unsigned char sizex, sizey;

/*unsigned char ctk_draw_windowborder_height = 1;
unsigned char ctk_draw_windowborder_width = 1;
unsigned char ctk_draw_windowtitle_height = 1;*/

// The revers function does nothing in our conio driver anyway.
#define revers(x)

/*-----------------------------------------------------------------------------------*/
static void
s_ctk_draw_init(void)
{
  (void)bgcolor(SCREENCOLOR);
  (void)bordercolor(BORDERCOLOR);
  clrscr();
  screensize(&sizex, &sizey);
}
/*-----------------------------------------------------------------------------------*/


static void customchr(const unsigned char* data) __naked __z88dk_callee
{
    __asm
	pop de
	pop hl
	push de
	; Cant use SCR SET MATRIX because some of our icons are in RAM under 0x4000.
	; SCR SET MATRIX then gets data from the firmware ROM...
	ld a,#0x19
	call 0xBB5a
	ld a,#0xff
	call 0xBB5a
	ld b,#8
00001$:
	ld a,(hl)
	call 0xbb5a
	inc hl
	djnz 00001$
	ret
    __endasm;
}


static void
draw_widget(struct ctk_widget *w,
	    unsigned char x, unsigned char y,
	    unsigned char clipx,
	    unsigned char clipy,
	    unsigned char clipy1, unsigned char clipy2,
	    unsigned char focus)
{
  unsigned char xpos, ypos, xscroll;
  unsigned char i, j;
  char c;
  const char* text;
  unsigned char len, wfocus;

  wfocus = 0;
  if(focus & CTK_FOCUS_WINDOW) {    
    (void)textcolor(WIDGETCOLOR_FWIN);
    if(focus & CTK_FOCUS_WIDGET) {
      (void)textcolor(WIDGETCOLOR_FOCUS);
      wfocus = 1;
    }
  } else if(focus & CTK_FOCUS_DIALOG) {
    (void)textcolor(WIDGETCOLOR_DIALOG);
    if(focus & CTK_FOCUS_WIDGET) {
      (void)textcolor(WIDGETCOLOR_FOCUS);
      wfocus = 1;
    }
  } else {
    (void)textcolor(WIDGETCOLOR);
  }
  
  xpos = x + w->x;
  ypos = y + w->y;
    
  switch(w->type) {
  case CTK_WIDGET_SEPARATOR:
    if(ypos >= clipy1 && ypos < clipy2) {
      chlinexy(xpos, ypos, w->w);
    }
    break;
  case CTK_WIDGET_LABEL:
    text = w->widget.label.text;
    for(i = 0; i < w->h; ++i) {
      if(ypos >= clipy1 && ypos < clipy2) {
	gotoxy(xpos, ypos);
	cputsn(text, w->w);
	if(w->w - (wherex() - xpos) > 0) {
	  cclear(w->w - (wherex() - xpos));
	}
      }
      ++ypos;
      text += w->w;
    }
    break;
  case CTK_WIDGET_BUTTON:
    if(ypos >= clipy1 && ypos < clipy2) {
      revers(wfocus != 0);
      cputcxy(xpos, ypos, '[');
      cputsn(w->widget.button.text, w->w);
      cputc(']');
      revers(0);
    }
    break;
  case CTK_WIDGET_HYPERLINK:
    if(ypos >= clipy1 && ypos < clipy2) {
      (void)textcolor(WIDGETCOLOR_HLINK);
      revers(wfocus == 0);
      gotoxy(xpos, ypos);
      cputsn(w->widget.button.text, w->w);
      revers(0);
    }
    break;
  case CTK_WIDGET_TEXTENTRY:
    text = w->widget.textentry.text;
    xscroll = 0;
    if(w->widget.textentry.xpos >= w->w - 1) {
      xscroll = w->widget.textentry.xpos - w->w + 1;
    }
    for(j = 0; j < w->h; ++j) {
      if(ypos >= clipy1 && ypos < clipy2) {
	if(w->widget.textentry.state == CTK_TEXTENTRY_EDIT &&
	   w->widget.textentry.ypos == j) {
	  revers(0);
	  cputcxy(xpos, ypos, '>');
	  c = 1;
	  for(i = 0; i < w->w; ++i) {
	    if(c != 0) {
	      c = text[i + xscroll];
	    }
            revers(i == w->widget.textentry.xpos - xscroll);
	    if(c == 0) {
	      cputc(' ');
	    } else {
	      cputc(c);
	    }
	  }
	  revers(0);
	  cputc('<');
	} else {
	  revers(wfocus != 0 && j == w->widget.textentry.ypos);
	  cvlinexy(xpos, ypos, 1);
	  gotoxy(xpos + 1, ypos);          
	  cputsn(text, w->w);
	  i = wherex();
	  if(i - xpos - 1 < w->w) {
	    cclear(w->w - (i - xpos) + 1);
	  }
	  cvline(1);
	}
      }
      ++ypos;
      text += w->widget.textentry.len + 1;
    }
    revers(0);
    break;
  case CTK_WIDGET_ICON:

    if(ypos >= clipy1 && ypos < clipy2) {
      revers(wfocus != 0);
#if CTK_CONF_ICON_BITMAPS
      gotoxy(xpos, ypos);
      if(w->widget.icon.bitmap != NULL) {
	  const unsigned char* ptr  = w->widget.icon.bitmap;
	for(i = 0; i < 3; ++i) {
	  gotoxy(xpos, ypos);
	  if(ypos >= clipy1 && ypos < clipy2) {
	    customchr(ptr);
	    cputc(0xff);
	    ptr += 8;
	    customchr(ptr);
	    cputc(0xff);
	    ptr += 8;
	    customchr(ptr);
	    cputc(0xff);
	    ptr += 8;
	  }
	  ++ypos;
	}
      }
#elif CTK_CONF_ICON_TEXTMAPS
      gotoxy(xpos, ypos);
      if(w->widget.icon.textmap != NULL) {
	for(i = 0; i < 3; ++i) {
	  gotoxy(xpos, ypos);
	  if(ypos >= clipy1 && ypos < clipy2) {
	    cputc(w->widget.icon.textmap[0 + 3 * i]);
	    cputc(w->widget.icon.textmap[1 + 3 * i]);
	    cputc(w->widget.icon.textmap[2 + 3 * i]);
	  }
	  ++ypos;
	}
      }
#endif
      x = xpos;
  
      len = strlen(w->widget.icon.title);
      if(x + len >= sizex) {
	x = sizex - len;
      }

      gotoxy(x, ypos);
      if(ypos >= clipy1 && ypos < clipy2) {
	cputs(w->widget.icon.title);
      }
      revers(0);
    }
    break;

  default:
    break;
  }
}
/*-----------------------------------------------------------------------------------*/
static void
s_ctk_draw_widget(struct ctk_widget *w,
		  unsigned char focus,
		  unsigned char clipy1,
		  unsigned char clipy2)
{
  struct ctk_window *win = w->window;
  unsigned char posx, posy;

  posx = win->x + 1;
  posy = win->y + 2;

  if(w == win->focused) {
    focus |= CTK_FOCUS_WIDGET;
  }
  
  draw_widget(w, posx, posy,
	      posx + win->w,
	      posy + win->h,
	      clipy1, clipy2,
	      focus);
  
#ifdef CTK_CONIO_CONF_UPDATE
  CTK_CONIO_CONF_UPDATE();
#endif /* CTK_CONIO_CONF_UPDATE */
}
/*-----------------------------------------------------------------------------------*/

static void clearrect(unsigned char y2, unsigned char x2,
	unsigned char y1, unsigned char x1) __naked
{
  __asm
		pop bc ; RV
		pop de ; x2 y2
		pop hl ; x1 y1
		push hl
		push de
		push bc

		call	0xBB99 ; TXT GET PAPER
		call	0xBC2C ; SCR INK ENCODE

		jp	0xBC44 ; SCR FILL BOX
    __endasm;
}

static void
s_ctk_draw_clear_window(struct ctk_window *window,
		      unsigned char focus,
		      unsigned char clipy1,
		      unsigned char clipy2)
{
  unsigned char i;
  unsigned char h;

  /*
  if(focus & CTK_FOCUS_WINDOW) {
    (void)textcolor(WINDOWCOLOR_FOCUS);
  } else {
    (void)textcolor(WINDOWCOLOR);
  }
  */
    
  i = window->y + 2; // +1 for the border, +1 for ctk > cpc conversion
  h = i + window->h;

  if (i >= clipy2 || h < clipy1)
	  return;

  if (i < clipy1) i = clipy1;
  if (h >= clipy2) h = clipy2 - 1;

  clearrect(h, window->x + window->w, i, window->x + 1);
}
/*-----------------------------------------------------------------------------------*/
static void
draw_window_contents(struct ctk_window *window, unsigned char focus,
		     unsigned char clipy1, unsigned char clipy2,
		     unsigned char x1, unsigned char x2,
		     unsigned char y1, unsigned char y2)
{
  struct ctk_widget *w;
  unsigned char wfocus;
  
  /* Draw inactive widgets. */
  for(w = window->inactive; w != NULL; w = w->next) {
    draw_widget(w, x1, y1, x2, y2,
		clipy1, clipy2,
		focus);
  }
  
  /* Draw active widgets. */
  for(w = window->active; w != NULL; w = w->next) {  
    wfocus = focus;
    if(w == window->focused) {
      wfocus |= CTK_FOCUS_WIDGET;
    }

   draw_widget(w, x1, y1, x2, y2, 
	       clipy1, clipy2,
	       wfocus);
  }

#ifdef CTK_CONIO_CONF_UPDATE
  CTK_CONIO_CONF_UPDATE();
#endif /* CTK_CONIO_CONF_UPDATE */
}
/*-----------------------------------------------------------------------------------*/
static void
s_ctk_draw_window(struct ctk_window *window, unsigned char focus,
		  unsigned char clipy1, unsigned char clipy2,
		  unsigned char draw_borders)
{
  unsigned char x, y;
  unsigned char h;
  unsigned char x1, y1, x2, y2;

  if(window->y + 1 >= clipy2) {
    return;
  }
    
  x = window->x;
  y = window->y + 1;
  x1 = x + 1;
  y1 = y + 1;
  x2 = x1 + window->w;
  y2 = y1 + window->h;

  if(draw_borders) {  

    /* Draw window frame. */
    if(focus & CTK_FOCUS_WINDOW) {
      (void)textcolor(WINDOWCOLOR_FOCUS);
    } else {
      (void)textcolor(WINDOWCOLOR);
    }

    if(y >= clipy1) {
      cputcxy(x, y, CH_ULCORNER);
      gotoxy(wherex() + window->titlelen + CTK_CONF_WINDOWMOVE * 2, wherey());
      chline(window->w - (wherex() - x) - 2);
      cputcxy(x2, y, CH_URCORNER);
    }

    h = window->h;
  
    if(clipy1 > y1) {
      if(clipy1 - y1 < h) {
	h = clipy1 - y1;
	      y1 = clipy1;
      } else {
	h = 0;
      }
    }

    if(clipy2 < y1 + h) {
      if(y1 >= clipy2) {
	h = 0;
      } else {
	h = clipy2 - y1;
      }
    }

    cvlinexy(x, y1, h);
    cvlinexy(x2, y1, h);  

    if(y + window->h >= clipy1 &&
       y + window->h < clipy2) {
      cputcxy(x, y2, CH_LLCORNER);
      chlinexy(x1, y2, window->w);
      cputcxy(x2, y2, CH_LRCORNER);
    }
  }

  draw_window_contents(window, focus, clipy1, clipy2,
		       x1, x2, y + 1, y2);
}
/*-----------------------------------------------------------------------------------*/
static void
s_ctk_draw_dialog(struct ctk_window *dialog)
{
  unsigned char x, y;
  unsigned char x1, y1, x2, y2;
  
  (void)textcolor(DIALOGCOLOR);

  x = dialog->x;
  y = dialog->y + 1;

  x1 = x + 1;
  y1 = y + 1;
  x2 = x1 + dialog->w;
  y2 = y1 + dialog->h;

  /* Draw dialog frame. */
  cvlinexy(x, y1,
	   dialog->h);
  cvlinexy(x2, y1,
	   dialog->h);

  chlinexy(x1, y,
	   dialog->w);
  chlinexy(x1, y2,
	   dialog->w);

  cputcxy(x, y, CH_ULCORNER);
  cputcxy(x, y2, CH_LLCORNER);
  cputcxy(x2, y, CH_URCORNER);
  cputcxy(x2, y2, CH_LRCORNER);
  
  /* Clear dialog contents. */
  clearrect(y2 - 1, x1 + dialog->w - 1, y1, x1);

  draw_window_contents(dialog, CTK_FOCUS_DIALOG, 0, sizey,
		       x1, x2, y1, y2);
}
/*-----------------------------------------------------------------------------------*/
static void
s_ctk_draw_clear(unsigned char y1, unsigned char y2) __naked
{
  __asm
	  pop de
	  pop hl
	  push hl
	  push de

		push af

		ld		e,h
		ld		h,#1
		ld d,#40

		dec e

		call	0xBB99 ; TXT GET PAPER
		call	0xBC2C ; SCR INK ENCODE

		call	0xBC44 ; SCR FILL BOX

		pop af
		ret
    __endasm;
}
/*-----------------------------------------------------------------------------------*/
static void
draw_menu(struct ctk_menu *m, unsigned char open)
{
  unsigned char x, x2, y;

  if(open) {
    x = x2 = wherex();
    if(x2 + CTK_CONF_MENUWIDTH > sizex) {
      x2 = sizex - CTK_CONF_MENUWIDTH;
    }

    for(y = 0; y < m->nitems; ++y) {
      if(y == m->active) {
	(void)textcolor(ACTIVEMENUITEMCOLOR);
      } else {
	(void)textcolor(SCREENCOLOR);	  
      }
      gotoxy(x2, y + 1);
      if(m->items[y].title[0] == '-') {
	chline(CTK_CONF_MENUWIDTH);
      } else {
	cputs(m->items[y].title);
      }
      if(x2 + CTK_CONF_MENUWIDTH > wherex()) {
	cclear(x2 + CTK_CONF_MENUWIDTH - wherex());
      }
    }

    gotoxy(x, 0);
    (void)textcolor(OPENMENUCOLOR);
  }

  cputs(m->title);
  cputc(' ');
}
/*-----------------------------------------------------------------------------------*/
static void
s_ctk_draw_menus(struct ctk_menus *menus)
{
  struct ctk_menu *m;

  /* Draw menus */
  (void)bgcolor(MENUCOLOR);
  (void)textcolor(SCREENCOLOR);	  
  gotoxy(0, 0);
  cputc(' ');
  for(m = menus->menus->next; m != NULL; m = m->next) {
    draw_menu(m, m == menus->open);
  }

  /* Draw desktopmenu */
  if(wherex() + strlen(menus->desktopmenu->title) + 1 >= sizex) {
    gotoxy(sizex - strlen(menus->desktopmenu->title) - 1, 0);
  } else {
    cclear(sizex - wherex() -
	   strlen(menus->desktopmenu->title) - 1);
  }
  draw_menu(menus->desktopmenu, menus->desktopmenu == menus->open);

  (void)bgcolor(SCREENCOLOR);
}
/*-----------------------------------------------------------------------------------*/
static unsigned char
s_ctk_draw_height(void)
{
  return sizey;
}
/*-----------------------------------------------------------------------------------*/
static unsigned char
s_ctk_draw_width(void)
{
  return sizex;
}
/*-----------------------------------------------------------------------------------*/
static unsigned short
s_ctk_mouse_xtoc(unsigned short x)
{
  return x / 8;
}
/*-----------------------------------------------------------------------------------*/
static unsigned short
s_ctk_mouse_ytoc(unsigned short y)
{
  return y / 8;
}
/*-----------------------------------------------------------------------------------*/
static const struct ctk_draw_service_interface interface =
  {CTK_DRAW_SERVICE_VERSION,
   1,
   1,
   1,
   s_ctk_draw_init,
   s_ctk_draw_clear,
   s_ctk_draw_clear_window,
   s_ctk_draw_window,
   s_ctk_draw_dialog,
   s_ctk_draw_widget,
   s_ctk_draw_menus,
   s_ctk_draw_width,
   s_ctk_draw_height,
   s_ctk_mouse_xtoc,
   s_ctk_mouse_ytoc,
  };

EK_EVENTHANDLER(eventhandler, ev, data);
EK_PROCESS(proc, CTK_DRAW_SERVICE_NAME ": text", EK_PRIO_NORMAL,
	   eventhandler, NULL, (void *)&interface);

/*--------------------------------------------------------------------------*/
LOADER_INIT_FUNC(ctk_conio_service_init, arg)
{
  s_ctk_draw_init();
  ek_service_start(CTK_DRAW_SERVICE_NAME, &proc);
}
/*--------------------------------------------------------------------------*/
EK_EVENTHANDLER(eventhandler, ev, data)
{
  EK_EVENTHANDLER_ARGS(ev, data);
  
  switch(ev) {
  case EK_EVENT_INIT:
  case EK_EVENT_REPLACE:
    s_ctk_draw_init();
    ctk_restore();
    break;
  case EK_EVENT_REQUEST_REPLACE:
    ek_replace((struct ek_proc *)data, NULL);
    LOADER_UNLOAD();
    break;    
  }
}
/*--------------------------------------------------------------------------*/
