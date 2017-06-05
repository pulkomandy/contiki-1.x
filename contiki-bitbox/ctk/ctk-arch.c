/*
 * Copyright 2017, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under terms of the MIT license.
 */


#include "ctk-arch.h"

#include "lib/textmode/textmode.h"

struct event e = {0};

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

void
ctk_arch_draw_char(char c,
		   unsigned char x, unsigned char y,
		   unsigned char reversed,
		   unsigned char color)
{
	vram[y][x] = c;
	vram_attr[y][x] = color;
}
