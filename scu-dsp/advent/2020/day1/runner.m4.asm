        ;;
        ;; transfer to vdp2
        ;;

        ;; vdp2 address calculation
        mvi ((8 * 0x4000 + (64 - 8) * 4 + 0x05e00000) >> 2),wa0
                                   mov 0,ct3
        ;;                 clr a      mov 0,ct3
        ;; add             mov alu,a
        ;;                            mov all,wa0

        ;; end vdp2 address calculation
        dma1 mc3,d0,8
dma_wait:
        jmp t0,dma_wait
        nop
