;; File: crt0.s
;; Generic crt0.s for a Z80
;; From SDCC..
;; Modified to suit execution on the Amstrad CPC!
;; by H. Hansen 2003

    .module crt0
	.globl	_main
	.globl _progend
	.area	_HEADER (ABS)
	;; Ordering of segments for the linker.
	.area	_CODE
init:

;; Initialise global variables
    call    gsinit
	call	_main

_exit::
	ret

	.area _HOME
	.area _CODE
	.area _INITIALIZER
	.area   _GSINIT (REL)
    .area   _GSFINAL (REL)

	.area _INITIALIZED
	.area _DATA
	.area _BSEG
	.area _BSS (REL)
    .area _HEAP (REL)
_progend::
	
	.area _GSINIT (REL)
gsinit::	
	ld	bc, #l__INITIALIZER
	ld	a, b
	or	a, c
	jr	Z, gsinit_next
	ld	de, #s__INITIALIZED
	ld	hl, #s__INITIALIZER
	ldir
gsinit_next:
; Clear BSS sections
	ld hl,#s__DATA
	ld (hl),#0
	ld de,#s__DATA
	inc de
	ld bc,#l__DATA
	ldir

; Initialize disk ROM
	ld hl,#0xabff
	ld de,#0x40
	ld c,#7
	call 0xbcce

	.area   _GSFINAL
	; After the ROMs are initialized, initialize the heap.
	 jp __sdcc_heap_init
