        ;;
        ;; runner
        ;;

        ;; argument: m0, m1
        include(`solution.asm')
        ;; fallthrough: solution in A

        ;;   [X     ]   [Y     ]   [D1            ]
                                   mov 0,ct2
                                   mov all,mc2

        ;;
        ;; libraries
        ;;

        ;; argument: m2[0]
        include(`lib/unparse_base10.asm')
        ;; fallthrough: base10 characters in m3[56:63]

        jmp vdp2_display
        ;;   [X     ]   [Y     ]   [D1            ]
                                   mov (64 - 8),ct3 ; delay slot

        ;; argument: Y
        include(`lib/div10_unsigned.asm')
        ;; return: A

        ;;
        ;; transfer to vdp2
        ;;
vdp2_display:
        ;; vdp2 address calculation
        mvi ((8 * 0x4000 + (64 - 8) * 4 + 0x05e00000) >> 2),wa0
        ;;   [X     ]   [Y     ]   [D1            ]
        ;; end vdp2 address calculation
        dma1 mc3,d0,8
dma_wait:
        jmp t0,dma_wait
        nop

        ;;
        ;; end of program
        ;;
        endi
        nop
