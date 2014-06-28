;; File: crt0.s
;; Generic crt0.s for a Z80
;; From SDCC..
;; Modified to suit execution on the Amstrad CPC!
;; by H. Hansen 2003

    .module crt0
	.globl	_main
;	.globl __jpbc
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
;__jpbc:
;	push bc
;	ret

	.area _HOME
	.area _CODE
	.area _INITIALIZER
	.area   _GSINIT (REL)
    .area   _GSFINAL (REL)
_progend::

	.area	_DATA (REL)
	.area _INITIALIZED
	.area _BSEG
	 .area   _BSS (REL)
    	.area   _HEAP (REL)
	
	.area   _GSINIT (REL)
gsinit::	
	ld	bc, #l__INITIALIZER
	ld	a, b
	or	a, c
	jr	Z, gsinit_next
	ld	de, #s__INITIALIZED
	ld	hl, #s__INITIALIZER
	ldir
gsinit_next:

	.area   _GSFINAL
	ret
