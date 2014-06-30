Contiki-1.x
===========

Contiki is a small operating system for embedded devices. While version 2 of
the system is designed to run on embedded devices and has an IP and IPv6 stack
as the main feature, the 1.x version of the system is better known for being
ported to several 8-bit and 16-bit home computers.

Contiki 1.x features a GUI, dynamic loading of executables with runtime
relocation, and a cooperative multitasking event-driven kernel. It also includes
an IPv4 network stack and a few other things.

This fork is focused on improving the Amstrad CPC port of Contiki. This version
was done by Kevin Thacker, but he didn't get it much further than showing the
desktop. At the time, problems with the SDCC compiler and lack of proper
optimization support led to a Contiki kernel too big and slow to be useful for
serious use.

Fast forward some years, and SDCC has improved a lot. While it's still not
very good at generating fast code, at least the size is down a bit and we now
can run several programs without running out of memory. The linker scripts you
will find here were modified to work properly with the current version of SDCC.

However, the dynamic relocatable executables are generated with a patched
version of the SDCC linker, as the existing linker doesn't allow output in a
suitable format.

Compared to the binaries released by Kevin Thacker, this version is a bit faster
because of slightly improved screen drawing routines. The performance is still
far from optimal, but we have a plan to improve this (see the roadmap below).
Also, reverse video support was disabled because the way it is done in Contiki
and in the CPC firmware are not directly compatible.

How to build it
===============

Requirements
------------

You will need a patched version of SDCC. The linker was modified to generate
relocation information, so the PRG executables can be loaded anywhere in memory
and relocated at runtime before starting them. Running Contiki without that on
the CPC would be much less interesting, because it is nearly impossible to write
position independant z80 code.

A patch for SDCC 3.4.1 (from the current SVN sources) is provided. Get the
sources using SVN or a nightly snapshot and apply the patch, then configure
SDCC as usual.

You can still use the generated version of SDCC for other projects. The only
difference is the addition of the -h flag to the linker. When this flag is set,
executables are generated with relocation information.

You will also need cpcgs from the cpctools project.

Steps
-----

Once the patched SDCC is installed, the process is rather simple:

  cd contiki-cpc
  make clean
  make cpc
  make programs

This will generate a dsk image with contiki and the various programs.

Be careful to always do things in this order. The "cpc" target compiles the
contiki core, and generate a defines file which is then used to have the apps
call contiki routines. However, when contiki is recompiled, stuff move in
memory and all programs must be recompiled. This means you should always do
a "make clean", until the dependencies are properly defined in the makefiles.

How to use it
=============

Boot your CPC or emulator and insert the disk in drive A (drive B is currently
not supported). Then from the BASIC prompt type

  run"contiki

The Contiki desktop will start, and will load the "Welcome" program which shows
a window with some hints about how to use the system. Once there, you can:

 * Navigate the menus (press F1 then use arrow keys)
 * Run the "Processes" program to see a list of running processes
 * Run the "Directory" program to list the disc contents

Using either Directory or the "Run program" menu, you can start more applications,
such as the calculator, the command line shel, the about box, etc. You can start
multiple instances of each application, and navigate between their windows using
the "Desktop" menu.

Roadmap
=======

This port of Contiki is running fairly well, but we can make it more awesome!

Current status
--------------

Contiki currently relies on the CPC firmware for screen drawing and on AMSDOS
for disc access. It runs entirely in the 64K base memory and doesn't use the
banks or other expansion ROMs.

Contiki uses the space usually reserved to BASIC, from &100 to &3700, for its
kernel. Since the Firmware and AMSDOS reserve all memory from &A700 up, this 
leaves about 28K of free RAM for applications. Not bad, but we can do better.

Firmware-based CTK driver
-------------------------

The screen driver is using the standard "conio" driver from Contiki. This is a
textmode based driver which is easily portable between different terminal types.
However, the interface of this driver with the CPC firmware results in rather
slow screen drawing. The main reason is that some operations (such as erasing
or scrolling part of the screen) are done character by character, instead of
using the firmware functions which are much faster. Moreover, the portable conio
code is written in C, and replacing it with an assembler version would provide
another speed boost.

Some extra features such as bitmap icons, a custom character set and more can
be implemented here.

Make use of memory banks
------------------------

We can put Contiki in bank C7 and map it in C1 mode. This would free all the low
memory for apps. When calling the firmware, we can either use "far calls" so the
bank can be unmapped while drawing, or use mode C3 and tell the firmware to draw
at address 4000.

Note that the firmware calls are designed not to take direct memory pointers
most of the time (eg you can print a single character, not a whole string) to
make such schemes workable: The firmware would never need to directly access
application memory in the range 4000-7fff. This would leave about 42K of RAM
free for apps.

Remove dependencies on firmware
-------------------------------

The next step is to completely remove the dependency on the firmware, and instead
write our own screen drawing routines. A 4x8 or 6x8 font could be used, as the
"80 column" version of Contiki for C64 is doing.

This could further speedup the screen display and allow for a nicer look.

Overscan display
----------------

A nice feature on the CPC is the ability to allocate 32K of RAM for the display
and have a quite high resolution screen (380x272 or so). However, with the
scheme exposed above this would lead to having only 32K of RAM free for applications.

To avoid this, we would run Contiki in C2 banking mode (all memory is mapped in
banks) and have the application heap there. Contiki would still be in bank C7
leaving 48K of RAM for apps. When drawing to the screen is needed, Contiki can
switch to mode C1 or C3 to access the main memory. A scheme similar to the one
used by the firmware needs to be used here: the screen drawing routines must
not do direct access to applications.

Pages 0 and 1 in main RAM would be used for the screen. Page 2 will have the
screen drawing code. Page 3 can be used for the filesystem, and use the C4-C7
mapping mode to access the banks. When using these modes, converting a pointer
to page number + pointer in 4000-7fff is easy. It may be a good idea to tweak
malloc so it never allocates a chunk that crosses two banks. But that would mean
we can't load apps bigger than 16K. So the disk system will probably have to
figure out how to handle allocations that spans two or more banks.

Even more free RAM!
-------------------

Going even further, Contiki should all be in main RAM, and leave the banks
almost completely free for apps. This would need to use an RST (far call or so)
to call Contiki methods from apps. Can SDCC handle this? We may need to generate
syscall inlines or maybe we can do dirty tricks using the peephole to replace
"CALL address" with "RST farcall ; dw address". This could leave 63+K of RAM
for apps, and 32K of RAM for Contiki + screen drawing + FS. If space is scarce,
it's probably time we try putting Contiki in one or two ROMs instead.

This is similar to the scheme used by CP/M+.

PCW port
--------

The amstrad PCW has a similar, but more flexible, RAM bank system. However, it
comes with 256 or 512K of memory, and we must support this!. This means reworking
Contiki to handle apps in the different banks, which is not an easy task and
may need compiler specific support. But then again, it could be useful for a
Thomson MO6/TO8 port...
