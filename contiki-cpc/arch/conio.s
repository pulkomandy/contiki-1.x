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
		call 	0xBC0E	; SCR SET MODE

		; BACKGROUND
		XOR A
		LD BC,#0x0D0D ; GREY
		CALL 0xBC32 ; SCR SET INK

		; BORDERS
		LD A,#1
		LD BC,#0x0e0e ; BLUE
		CALL 0xBC32 ; SCR SET INK

		; WIDGETS
		LD A,#2
		LD BC,#0x1a1a ; WHITE
		CALL 0xBC32 ; SCR SET INK

		; FOCUS
		LD A,#3
		LD BC,#0x0000 ; BLACK
		CALL 0xBC32 ; SCR SET INK

		LD DE,#255
		LD HL,#0xbe80
		jp 0xbbab ; TXT SET M TABLE


; void gotox (unsigned char x);
; Set the cursor to the specified X position, leave the Y position untouched 

.globl _gotox

_gotox::
		ld a,l
		inc a
		jp	0xBB6F	; TXT SET COLUMN

; void gotoy (unsigned char y);
; Set the cursor to the specified Y position, leave the X position untouched

.globl _gotoy

_gotoy::
		ld a,l
		inc		a
		jp	0xBB72	; TXT SET ROW

; void gotoxy (unsigned char x, unsigned char y)
; Set the cursor to the specified position 
; y pushed first, then x

.globl _gotoxy

_gotoxy::
		pop hl
		pop de
		push de
		push hl

		ld l,d
		ld h,e

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
		ld a,l
		jp		0xbb5d	; TXT OUTPUT


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

; void cputsn(char *str, unsigned char len);
.globl _cputsn

_cputsn::
	pop de ; RV
	pop hl ; str
	pop bc ; len
	push bc
	push hl
	push de

	LD B,C
cputsn$:
	LD A,(HL)
	INC HL
	OR A
	RET Z
	PUSH HL
	PUSH BC
	CALL 0xBB5D
	POP BC
	POP HL
	DJNZ cputsn$
	RET

; void cputs (const char* s);
; Output a NUL terminated string at the current cursor position 
; TESTED

.globl _cputs

_cputs::
cputs$:
		ld		a,(hl)
		inc hl
		or a
		ret		z
		push hl
		call	0xbb5d
		pop hl
		jr		cputs$

; void textcolor (unsigned char color);
; Set the color for text output.
.globl	_textcolor

_textcolor::
		ld		a,l
		jp	0xBB90	; TXT SET PEN


; void bgcolor (unsigned char color);
; Set the color for the background. */
.globl	_bgcolor

_bgcolor::	
		ld		a,l
		jp	0xBB96   ; TXT SET PAPER

; void bordercolor (unsigned char color);
; Set the color for the border.

.globl	_bordercolor

_bordercolor::

		ld		b,l
		ld c,l
		jp	0xBC38   ; SCR SET BORDER

; void chline (unsigned char length);
; Output a horizontal line with the given length starting at the current
; cursor position.

.globl	_chline

_chline::	
		ld		a,l
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

		POP HL ; RV
		POP DE ; XY
		POP BC ; L
		PUSH BC
		PUSH DE
		PUSH HL

		ld h,e
		ld l,d
		ld b,c

		inc h
		inc l
		call	0xBB75
		jr dochline$

; void cvline (unsigned char length);
; Output a vertical line with the given length at the current cursor
; position.

.globl _cvline

_cvline::	
		ld		a,l
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
		POP BC ; RV
		POP DE ; XY
		POP HL ; L.
		PUSH HL
		PUSH DE
		PUSH BC

		LD B,L
		LD L,D
		LD H,E

		inc 		h
		inc 		l
		jr docvline$

; void cclear (unsigned char length);
; Clear part of a line (write length spaces).

.globl _cclear

_cclear::	
		ld		b,l
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
		pop bc ; RV
		pop de ; ptr1
		pop hl ; ptr2

		ld		a,#40    ; X Size
		ld		(de),a

		ld		a,#25    ; Y Size
		ld		(hl),a

		push hl
		push de
		push bc

		ret

