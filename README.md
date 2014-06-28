contiki-1.x
===========

The historical Contiki 1.x sources

This fork is an attempt to get the CPC version of Contiki 1.x running again.
The port never left the proof of concept stage and could use some love.

The build has been fixed to work with a current SDCC (3.4) but currently the OS
(contiki.bin) will crash right after loading. Some debugging will be needed.

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

Contiki will take over the space allocated to BASIC and run there. Addresses
100-4500 are for the code and internal variables. Between 4500 and A7FF is the
heap memory, which is dynamically allocated to the different programs you may
want to run. The heap size depends on the Contiki program size (low address)
and use of memory by the CPC firmware and expansion ROMs (high address).

Contiki is firmware-friendly and uses the CPC firmware for all IO operations
(disk access, screen drawing, etc).
