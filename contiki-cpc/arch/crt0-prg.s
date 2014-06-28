;; File: crt0-prg.s

    .module crt0
	.area _HEADER (ABS)
	.area _HOME (REL)	
	.area _CODE (REL)
	.area _INITIALIZER
	.area   _GSINIT (REL)
    .area   _GSFINAL (REL)
	;; relocation data
	.dw 0
	
	.area _INITIALIZED
    .area _DATA (REL)
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
; Clear BSS sections
	ld hl,#s__DATA
	ld (hl),#0
	ld de,#s__DATA
	inc de
	ld bc,#l__DATA
	ldir

	
    .area   _GSFINAL (REL)
	ret
