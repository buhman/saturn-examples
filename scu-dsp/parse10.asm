        ;; input m0[0] start address
        ;;   [X       ]   [Y       ]   [D1       ]
        ;;                             mov 0,ct0
        ;;                             mov mc0,ra0

        ;; variables:
        ;; m0[0]
        ;; m0[1] dma read value (local); post-shift 4-byte remainder (global)
        ;; m0[2] saved accumulator (global)

setup:
        ;;   [X       ]   [Y       ]   [D1       ]
                                       mov 1,ct0
                                       mov 0,ct3
                                       mov 0,mc0
                                       mov 0,mc0
        mvi parse_base10,pc
                                       mov 1,lop
        ;; parse_base10 return
                                       mov all,mc3

        mvi parse_base10,pc
                                       mov 1,lop
        ;; parse_base10 return
                                       mov all,mc3

        nop
        nop
        nop

parse_base10:
        ;;   [X       ]   [Y       ]   [D1       ]
                                       mov 1,ct0
             mov m0,p     mov m0,a
        add
        jmp nz,parse_loop
                          clr a        ; delay slot
dma_next_chunk:
        ;; dma d0,mc0,1
        ;; begin fake number 0x39322033 "92 3"
        mvi 0x393220,pl
        add               mov alu,a    mov 0x33,pl
        rl8               mov alu,a    mov 1,ct0
        add                            mov all,mc0
        ;; end fake number
end_dma_next_chunk:
parse_loop:
        ;; extract first (leftmost) digit
                                       mov 1,ct0
                          mov mc0,a    mov 0x7f,pl
        or
        jmp s,end_of_input
        rl8               mov alu,a    mov 10,rx     ; m0[2] 0x00000000
        jmp z,dma_next_chunk
        and               mov m0,y     mov all,pl

        ;; MUL: post-multiplication accumulator
        ;; P: post-mask digit
        ;; A: pre-xor string
                                       mov 1,ct0
        xor               clr a        mov all,mc0 ; m0[1] 0x20323300

        add               mov alu,a    mov 0x30,pl ; 0x30 ascii '0'

        ;; A: post-mask digit
        ;;
        sub               mov alu,a    mov 10,pl

        ;; if ((0x39 - 0x30) < 0) goto non_digit;
        jmp s,non_digit
        ;; possibly set S flag for `jmp ns,non_digit`
        sub                            mov 2,ct0   ; sub in delay slot
        ;; if (((0x39 - 0x30) - 10) >= 0) goto non_digit;
        jmp ns,non_digit
                          mov mul,p    mov 2,ct0   ; mov in delay slot
        ;; P: accumulator * 10 result
digit:
        ;; we have a valid digit; add it to the accumulator; save to m0[2]
        ;;   [X       ]   [Y       ]   [D1       ]
        jmp parse_loop
        add                            mov all,mc0 ; add in delay slot ; m0[2] 0x00000009

non_digit:
        ;; this is not a valid digit; return and clear the accumulator as A
        ;;   [X       ]   [Y       ]   [D1       ]
        btm
                          mov m0,a     mov 0,mc0   ; clr in delay slot ; m0[2]

        ;; m0:
        ;; 00000000 32330000 00000009 0000000
end_of_input:
        btm
                          clr a
