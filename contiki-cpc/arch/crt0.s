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
; This is the program entry point. Execution starts here
;; Initialise global variables, clear BSS areas, initialize AMSDOS and setup heap.
    call    gsinit
; Enter the C main.
	call	_main

_exit::
	ret

	.area _HOME
	.area _CODE
	.area _INITIALIZED

	.area _DATA
	.area _BSEG
	.area _BSS (REL)
    .area _HEAP (REL)
_progend::
	; NOTE - THE IS NOT ROM-FRIENDLY!
	; We put the initializers for initialized data in the memory that will later
	; be used for the heap. The gsinit copies it back to the CODE area above.
	; Then, we can overwrite the now unused initializers with the heap.
	; In the case of software actually running from ROM, the initializer section
	; would be in ROM, and GSINIT would copy it to RAM. Of course in that case,
	; The initializer space can't be reclaimed for the heap...
	.area _INITIALIZER
	.area   _GSINIT (REL)
    .area   _GSFINAL (REL)
	
; -----------------------------------------------------------------------------

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
	ld de,#s__DATA + #1
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
