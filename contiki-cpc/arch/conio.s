;*****************************************************************************/
; CONIO.S - Amstrad CPC version of the Contiki conio.h (derived from
;borland C) ; To use with the Small Devices C Compiler ; ; 2003 H. Hansen
;*****************************************************************************/

;; contiki uses coordinates between 0..width-1, 0..height-1
;; cpc uses coordinates between 1..width, 1..height





; void clrscr (void);
; Clear the whole screen and put the cursor into the top left corner 
; TESTED

.globl _clrscr	
		.area _CODE

_clrscr::
		ld		a,#1
		jp 	0xBC0E	; SCR SET MODE


; void gotox (unsigned char x);
; Set the cursor to the specified X position, leave the Y position untouched 

.globl _gotox

_gotox::
		ld		hl,#2
		add		hl,sp
		ld		a,(hl)
		inc		a
		jp	0xBB6F	; TXT SET COLUMN

; void gotoy (unsigned char y);
; Set the cursor to the specified Y position, leave the X position untouched

.globl _gotoy

_gotoy::
		ld		hl,#2
		add		hl,sp
		ld		a,(hl)
		inc		a
		jp	0xBB72	; TXT SET ROW

; void gotoxy (unsigned char x, unsigned char y)
; Set the cursor to the specified position 
; y pushed first, then x

.globl _gotoxy

_gotoxy::
		ld		hl,#2
		add		hl,sp
		ld 		a,(hl)
		inc hl
		ld 		l,(hl)
		ld h,a
		inc h
		inc l
		jp	0xBB75	; TXT SET CURSOR

; unsigned char wherex (void);
; Return the X position of the cursor 

.globl _wherex

_wherex::
		call	0xBB78	; TXT GET CURSOR
		ld		l,h
		dec l
		ret

; unsigned char wherey (void);
; Return the Y position of the cursor 

.globl _wherey

_wherey::
		call	0xBB78	; TXT GET CURSOR
		dec l
		ret


; void cputc (char c);
; Output one character at the current cursor position

.globl _cputc

_cputc::
		ld		hl,#2
		add		hl,sp
		ld		a,(hl)
		jp		0xbb5b	; TXT OUTPUT


; void cputcxy (unsigned char x, unsigned char y, char c)
; Same as "gotoxy (x, y); cputc (c);"

.globl _cputcxy

_cputcxy::
		ld		hl,#4
		add		hl,sp
		ld		e,(hl)
		dec		hl
		ld		a,(hl)
		dec 		hl
		ld 		h,(hl)
		ld 		l,a
		inc h
		inc l
		call	0xBB75	; TXT SET CURSOR
		ld		a,e		
		jp	0xbb5d

; void cputs (const char* s);
; Output a NUL terminated string at the current cursor position 
; TESTED

.globl _cputs

_cputs::
		ld		hl,#2
		add		hl,sp
		ld		e,(hl)
		inc 		hl
		ld		d,(hl)
		
cputs$:
		ld		a,(de)
		inc de
		or a
		ret		z
		push de
		push af
		call	0xbb5d
		pop af
		pop de
		jr		cputs$

; void cputsxy (unsigned char x, unsigned char y, const char* s);
; Same as "gotoxy (x, y); puts (s);" 
; TESTED
.globl _cputsxy

_cputsxy::
		ld		hl,#2
		add		hl,sp
		ld		e,(hl)
		inc		hl
		ld		d,(hl)

		ld		a,(hl)
		inc 		hl
		ld		l,(hl)
		ld 		h,a		
		ex de,hl
		inc h
		inc l
		call	0xBB75	; TXT SET CURSOR

		jr		cputs$


; void revers (unsigned char onoff);
; Enable/disable reverse character display. This may not be supported by
; the output device.
; TESTED
.globl _revers

_revers::
	ret



; unsigned char textcolor (unsigned char color);
; Set the color for text output. The old color setting is returned. 
.globl	_textcolor

_textcolor::
		ld		hl,#2
		add		hl,sp
		ld		a,(hl)
		ld		d,a
		call	0xBB93  ; TXT GET PEN
		ld		e,a
		ld		a,d
		call	0xBB90	; TXT SET PEN
		ld		l,e
		ret


; unsigned char bgcolor (unsigned char color);
; Set the color for the background. The old color setting is returned. */
.globl	_bgcolor

_bgcolor::	
		ld		hl,#2
		add		hl,sp
		ld		a,(hl)
		ld		d,a
		call	0xBB99   ; TXT GET PAPER
		ld		e,a
		ld		a,d
		call	0xBB96   ; TXT SET PAPER
		ld		l,e
		ret

; unsigned char bordercolor (unsigned char color);
; Set the color for the border. The old color setting is returned. 

.globl	_bordercolor

_bordercolor::

		ld		hl,#2
		add		hl,sp
		ld		a,(hl)
		push	af
		call	0xBC3B   ; SCR GET BORDER
		pop		af
		ld		d,b
		ld		b,a
		ld		c,a
		push	de
		call	0xBC38   ; SCR SET BORDER
		pop		de
		ld		l,d
		ret

; void chline (unsigned char length);
; Output a horizontal line with the given length starting at the current
; cursor position.

.globl	_chline

_chline::	
		ld		hl,#2
		add		hl,sp
		ld		a,(hl)
		or		a
		ret		z
		ld		b,a
dochline$:
		ld c,#0x09a
chlineloop$:
		push bc
		ld a,c
		call	0xbb5d
		pop bc
		djnz    chlineloop$
		ret

; void chlinexy (unsigned char x, unsigned char y, unsigned char length);
; Same as "gotoxy (x, y); chline (length);"
; TESTED

.globl _chlinexy

_chlinexy::
		ld		hl,#2
		add		hl,sp
		ld		d,(hl)
		inc		hl
		ld		e,(hl)
		inc 		hl
		ld		a,(hl)
		or		a
		ret		z
		ld		b,a
		ld		h,d
		ld		l,e
		inc h
		inc l
		call	0xBB75
		jr dochline$

; void cvline (unsigned char length);
; Output a vertical line with the given length at the current cursor
; position.

.globl _cvline

_cvline::	
		ld		hl,#2
		add		hl,sp
		ld		a,(hl)
		or		a
		ret		z
		ld		b,a
		call	0xBB78  ; TXT GET CURSOR
docvline$:
		ld c,#0x095
cvloop$:
		push hl
		push bc
		call	0xBB75 ; TXT SET CURSOR
		pop bc
		push bc
		ld a,c
		call	0xbb5d
		pop bc
		pop hl
		inc l
		djnz	cvloop$
		ret

; void cvlinexy (unsigned char x, unsigned char y, unsigned char length);
; Same as "gotoxy (x, y); cvline (length);"

.globl _cvlinexy

_cvlinexy::	
		ld		hl,#2
		add		hl,sp
		ld		d,(hl)
		inc 		hl
		ld		e,(hl)
		inc		hl
		ld		a,(hl)
		or		a
		ret		z
		ld		b,a
		ld		h,d
		ld		l,e
		inc 		h
		inc 		l
		jr docvline$

; void cclear (unsigned char length);
; Clear part of a line (write length spaces).

.globl _cclear

_cclear::	
		ld		hl,#2
		add		hl,sp
		ld		b,(hl)
		ld		c,#0x020 ; White space
cclearloop$:
		push bc
		ld a,c
		call	0xbb5d
		pop bc
		djnz	cclearloop$
		ret

; void cclearxy (unsigned char x, unsigned char y, unsigned char length);
; Same as "gotoxy (x, y); cclear (length);"

.globl _cclearxy

_cclearxy::
		ld		hl,#2
		add		hl,sp
		ld		d,(hl) ; X
		inc		hl
		ld		e,(hl) ; Y
		inc		hl

		ld		a,(hl) ; Length

		; E is BOTTOM
		; LEFT TOP
		ld		h,d
		ld		l,e

		; RIGHT
		dec 	a
		add		d
		ld		d,a

		; ink mask
		call	0xBB99 ; TXT GET PAPER
		call	0xBC2C ; SCR INK ENCODE

		jp	0xBC44 ; SCR FILL BOX


; void screensize (unsigned char* x, unsigned char* y);
; Return the current screen size.

.globl _screensize

_screensize::

		ld		hl,#2
		add		hl,sp
		ld		e,(hl)
		inc		hl
		ld 		d,(hl)

		ld		a,#40    ; X Size
		ld		(de),a

		inc     hl
		ld		e,(hl)
		inc		hl
		ld 		d,(hl)

		ld		a,#25    ; Y Size
		ld		(de),a
		ret

