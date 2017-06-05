/*
 * Copyright 2017, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under terms of the MIT license.
 */

// Contiki includes
#include "ctk.h"
#include "program-handler.h"

#include "clock-conf.h"

// Applications
#include "netconf-dsc.h"
#include "dhcp-dsc.h"
#include "www-dsc.h"
#include "webserver-dsc.h"
#include "ftp-dsc.h"
#include "telnet-dsc.h"
#include "email-dsc.h"
#include "irc-dsc.h"
#include "editor-dsc.h"
#include "calc-dsc.h"
#include "processes-dsc.h"
#include "shell-dsc.h"
#include "about-dsc.h"

// Bitbox includes
#include "lib/textmode/textmode.h"

void conio_init(void)
{
	// Clear display and set a visible palette (default is black on black)
	clear();

	for (int i = 0; i < 256; i++)
		set_palette(i, 0, 0);

	set_palette(0, RGB(0, 0, 0), RGB(0, 0, 0));       // border (ok)
	set_palette(1, RGB(0, 0, 0), RGB(146, 146, 255)); // screen (ok)
	set_palette(3, RGB(0, 0, 0), RGB(128, 128, 128)); // open menu (ok)
	set_palette(4, RGB(0, 0, 0), RGB(255, 128, 128)); // active menu item (ok)
	set_palette(5, RGB(0, 0, 0), RGB(240,240,240));     // dialogs (ok)
	set_palette(6, RGB(100,100,255), RGB(255,128,128));   // highlighted hyperlinks (ok)
	set_palette(8, RGB(64,64,64), RGB(192,192,192)); // inactive window/widgets

	set_palette(2+16, RGB(0, 0, 0), RGB(255, 255, 255)); // menu (ok)
	set_palette(6+16, RGB(0, 0, 255), RGB(240,240,240));    // hyperlinks (ok)
	set_palette(7+16, RGB(0,0,0), RGB(255,128,128));  // focus widget

	set_palette(7, RGB(0,0,64), RGB(192,192,255)); // moving window


}

clock_time_t clock_time(void)
{
	return vga_frame;
}

void
log_message(const char *part1, const char *part2)
{
	  printf("%s%s\n", part1, part2);
}

void bitbox_main(void)
{
	ek_init();
	conio_init();
	ctk_init();
	program_handler_init();

#if 0
  program_handler_add(&netconf_dsc,   "Network setup", 1);
  program_handler_add(&dhcp_dsc,      "DHCP client",   1);
  program_handler_add(&www_dsc,       "Web browser",   1);
  program_handler_add(&webserver_dsc, "Web server",    1);
  program_handler_add(&ftp_dsc,       "FTP client",    1);
  program_handler_add(&telnet_dsc,    "Telnet",        1);
  program_handler_add(&email_dsc,     "E-mail",        1);
  program_handler_add(&irc_dsc,       "IRC client",    1);
#endif
	program_handler_add(&editor_dsc,    "Editor",        1);
	program_handler_add(&calc_dsc,      "Calculator",    1);
	program_handler_add(&processes_dsc, "Processes",     1);
	//program_handler_add(&shell_dsc,     "Command shell", 1);
	program_handler_add(&about_dsc,     "About Contiki", 0);

	// Call ek_run until everything is initialized. Then, load welcome.prg.
	while(1) {
		if(ek_run() == 0) {
			//program_handler_load("welcome.prg", NULL);
			//TODO we don't have a loader; so call the init func directly
			break;
		}
	}
	// Run the main loop.
	while(1) {
		if (ek_run() == 0) {
			// We are out of events to run, sleep a little.
			wait_vsync(1);
		}
	}
}
