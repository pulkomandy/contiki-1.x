/*
 * Copyright 2017, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under terms of the MIT license.
 */


#include "ctk-arch.h"

#include "lib/textmode/textmode.h"

struct event e = {0};

short mx, my;

/* Keyboard management */
ctk_arch_key_t
ctk_arch_getkey(void)
{
	while (e.type != evt_keyboard_press || e.kbd.sym < 8)
	{
		events_poll();
		e = event_get();
	}

	return e.kbd.sym;
}

unsigned char kbhit(void)
{
	events_poll();
	e = event_get();
	return e.type == evt_keyboard_press && e.kbd.sym >= 8;
}


/* Mouse management */
void ctk_mouse_init(void)
{
	mx = 50;
	my = 20;
}

unsigned char ctk_mouse_xtoc(unsigned short x)
{
	return x;
}

unsigned char ctk_mouse_ytoc(unsigned short y)
{
	return y;
}

void ctk_mouse_hide(void)
{
	vram_attr[ctk_mouse_ytoc(my)][ctk_mouse_xtoc(mx)] &= ~32;
}

void ctk_mouse_show(void)
{
	vram_attr[ctk_mouse_ytoc(my)][ctk_mouse_xtoc(mx)] |= 32;
}

unsigned short ctk_mouse_x(void)
{
	if (mouse_x != 0)
	{
		ctk_mouse_hide();
		mx += mouse_x; mouse_x = 0;
		if (mx < 0) mx = 0;
		if (mx >= LIBCONIO_SCREEN_WIDTH) mx = LIBCONIO_SCREEN_WIDTH - 1;
	}
	return mx;
}

unsigned short ctk_mouse_y(void)
{
	if (mouse_y != 0) {
		ctk_mouse_hide();
		my += mouse_y; mouse_y = 0;
		if (my < 0) my = 0;
		if (my >= LIBCONIO_SCREEN_HEIGHT) my = LIBCONIO_SCREEN_HEIGHT - 1;
	}
	return my;
}

unsigned char ctk_mouse_button(void)
{
	return mouse_buttons;
}


/* Character display */
void
ctk_arch_draw_char(char c,
		   unsigned char x, unsigned char y,
		   unsigned char reversed,
		   unsigned char color)
{
	vram[y][x] = c;
	vram_attr[y][x] = color;
}
