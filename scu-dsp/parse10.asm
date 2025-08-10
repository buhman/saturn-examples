        ;; input m0[0] start address
        ;;   [X       ]   [Y       ]   [D1       ]
        ;;                             mov 0,ct0
        ;;                             mov mc0,ra0

        ;; variables:
        ;; m0[0] dma read start address (global)
        ;; m0[1] dma read value (local); post-shift 4-byte remainder (global)
        ;; m0[2] saved accumulator (global)

caller_setup:
        mov 0,ct0
        ;; start address
        mov mc0,ra0
        mov 2,ct0
        ;; mov 0,mc0

parse_base10:
next_chunk:
                          clr a        mov 1,ct0
        ;; dma d0,mc0,1
        ;; begin fake number 0x39322033 "92 3"
        mvi 0x393220,pl
        add               mov alu,a    mov 0x33,pl
        rl8               mov alu,a
        add                            mov all,mc0
        ;; end fake number
post_dma_load:
        ;; extract first (leftmost) digit
                                       mov 1,ct0
                          mov mc0,a    mov 0x7f,pl
        rl8               mov alu,a    ;;mov all,mc0 ; m0[1] 0x20323339
        ;; multiply saved accumulator by 10
        ;;   [X       ]   [Y       ]   [D1       ]
                          mov m0,y     mov 10,rx     ; m0[2] 0x00000000
        and                            mov all,pl

        ;; MUL: post-multiplication accumulator
        ;; P: post-mask digit
        ;; A: pre-xor string
                                       mov 1,ct0
        xor               clr a        mov all,mc0 ; m0[1] 0x20323300

        ;; FIXME: if 0x00 jump to "end of input"?

        add               mov alu,a    mov 0x30,pl ; 0x30 ascii '0'

        ;; A: post-mask digit
        ;;
        sub               mov alu,a    mov 10,pl

        ;; if ((0x39 - 0x30) < 0) goto non_digit;
        jmp s,non_digit
        ;; possibly set S flag for `jmp ns,non_digit`
        sub                                        ; sub in delay slot
        ;; if (((0x39 - 0x30) - 10) >= 0) goto non_digit;
        jmp ns,non_digit
                          mov mul,p    mov 2,ct0   ; mov in delay slot
        ;; P: accumulator * 10 result
digit:
        ;; we have a valid digit; add it to the accumulator; save to m0[2]
        ;;   [X       ]   [Y       ]   [D1       ]
        jmp post_dma_load
        add                            mov all,mc0 ; add in delay slot ; m0[2] 0x00000009

non_digit:
        ;; this is not a valid digit; return the accumulator
        ;;   [X       ]   [Y       ]   [D1       ]
        btm
                          clr a                    ; clr in delay slot

        ;; m0:
        ;; 00000000 32330000 00000009 0000000
