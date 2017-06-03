/*
 CONIO.H - Console I/O library for the Amsrad CPC
 To use with the Small Devices C Compiler
 CONIO.H is a implementation of the Contiki CONIO.H (Derived from the borland CONIO.H)

 2003 H. Hansen
*/

#ifndef  __conio_h__
#define __conio_h__

#include <stdio.h>
#include <stdlib.h>
#include "arch.h"

#define CH_ULCORNER 0x096
#define CH_URCORNER 0x09c
#define CH_LLCORNER 0x093
#define CH_LRCORNER 0x099

void clrscr (void);

unsigned char kbhit (void);

void gotox (unsigned char x) __z88dk_fastcall;

void gotoy (unsigned char y) __z88dk_fastcall;

void gotoyx (unsigned char y, unsigned char x) __z88dk_callee;
#define gotoxy(x,y) gotoyx(y,x)

unsigned char wherex (void);

unsigned char wherey (void);

void cputc (char c) __z88dk_fastcall;

void cputcyx (unsigned char y, unsigned char x, char c) __z88dk_callee;
#define cputcxy(x,y,c) cputcyx(y,x,c)

void cputs (const char* s) __z88dk_fastcall;
void cputsn(const char *str, unsigned char len) __z88dk_callee;

char cgetc (void);

void  revers (char) __z88dk_fastcall;

void textcolor (unsigned char color) __z88dk_fastcall;

void bgcolor (unsigned char color) __z88dk_fastcall;

void  bordercolor (unsigned char color) __z88dk_fastcall;

void chline (unsigned char length) __z88dk_fastcall;

void chlineyx (unsigned char y, unsigned char x, unsigned char length) __z88dk_callee;
#define chlinexy(x,y,l) chlineyx(y,x,l)

void cvline (unsigned char length) __z88dk_fastcall;

void cvlineyx (unsigned char y, unsigned char x, unsigned char length) __z88dk_callee;
#define cvlinexy(x,y,l) cvlineyx(y,x,l)

void cclear (unsigned char length) __z88dk_fastcall;

void screensize (unsigned char* x, unsigned char* y);

#endif /* __conio_h__ */

