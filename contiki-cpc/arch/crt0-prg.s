;; File: crt0-prg.s

    .module crt0
	.area _HEADER (ABS)
	.area _HOME (REL)	
	.area _CODE (REL)
	.dw 0 ; Will be replaced with the address of the reloc. table by the linker.
	.area _INITIALIZER (REL)
izr::
	.area   _GSINIT (REL)
    .area   _GSFINAL (REL)
	;; relocation data will be inserted here. It is used at loading time,
	; then overriden by the initialization of the data area below.
	
	.area _INITIALIZED (REL)
izd::
    .area _DATA (REL)
dat::
	.area _BSEG
    .area   _BSS (REL)
    .area   _HEAP (REL)

	.area   _GSINIT (REL)
gsinit::	
	ld	bc, #l__INITIALIZER
	ld	a, b
	or	a, c
	jr	Z, gsinit_next
	ld	hl, #izr
	ld	de, #izd
	ldir
gsinit_next:
; Clear BSS sections
	ld hl,#dat
	ld (hl),#0
	ld de,#dat
	inc de
	ld bc,#l__DATA
	ldir

	
    .area   _GSFINAL (REL)
	ret
