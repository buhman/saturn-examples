        ;;
        ;; function: div10_unsigned
        ;;

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
