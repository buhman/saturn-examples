        mov 0,ct0               ; num
        mov 0,ct1               ; vdp2 address
        mov 0,ct3               ; output
        mvi 12345,mc0                            ; m0[0] (12345)

        ;; clear m3
        mov 7,lop
        lps
        mov 0,mc3
        mov 0,ct3

        ;; output: m3
base10_loop:
                                   mov 0,ct0
        mvi div10_unsigned,pc
        ;;   [X     ]   [Y     ]   [D1     ]
                        mov mc0,y  mov 1,lop     ; mvi imm,pc delay slot (executed twice)
        ;; after function return:
                                   mov all,mc0   ; m0[1] (1234)
                                   mov all,pl    ; ??? why can't this happen in the next instruction?
        ;; mod10: multiply by 10:
        sl              mov alu,a
        sl              mov alu,a
        sl              mov alu,a
        add             mov alu,a
        add             mov alu,a  mov 0,ct0     ; restore ct0 after 'mov all,mc1'

        ;;   a: 12340
                        mov mc0,a  mov all,pl

        ;;   a: 12345   m0[0]
        ;;   p: 12340

        ;; mod10: subtract (a - p)
        sub             mov alu,a

        ;;   a: 5

        ;; convert to vdp2 character
        mvi 16,pl
        add             mov alu,a

        ;; store digit in m3
             mov mc0,p  clr a      mov all,mc3   ; m0[1]

        ;;   p: 1234
        ;;   a: 0
        add             mov alu,a  mov 0,ct0

        ;;   a: 1234
        jmp nz,base10_loop
                                   mov all,mc0   ; jmp delay slot


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

        endi
        nop

        ;; argument: ry
        ;; return: a ← ry / 10
        ;; maximum ry is somewhere between 2 ^ 21 and 2 ^ 22
div10_unsigned:
        ;; 1 / 10 * (2 ^ 24) ~= 1677722 = 0x19999a
        mvi 1677722,rx
             mov mul,p  clr a
        ad2             mov alu,a

        ;;   ALH: [nmlkjihgfedcba9876543210________]
        ;;   ALL: [76543210________________________]

                        clr a      mov alh,pl ; alh is (a >> 16)
        add             mov alu,a

        ;;   ALL: [nmlkjihgfedcba9876543210________]

        ;; rotate left 24 requires fewer cycles than shift right 8
        rl8             mov alu,a
        rl8             mov alu,a
        rl8             mov alu,a

        ;;   ALL: [________nmlkjihgfedcba9876543210]

        ;; mask 24 bit result
        mvi 0xffffff,pl

        ;; return to caller ; reset ct0
        btm
        and             mov alu,a  mov 0,ct0
