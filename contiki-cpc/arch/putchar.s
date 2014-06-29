;; File: putchar.s
;; Modified to suit execution on the Amstrad CPC
;; by H. Hansen 2003
;; Original lines has been marked out!

		.area _CODE
_putchar::       
        ld      hl,#2
        add     hl,sp
        
        ld      a,(hl)
	
		ld bc,#0xef00
		out (c),c
		or  #0x80
		out (c),a

        ret

