        ;;
        ;; inline function: unparse_base10
        ;;

        ;; argument: m2[0]
        ;; return: base10 + 16 in m3[56 - 63]

        ;; requires: div10_unsigned

unparse_base10:
        ;; clear end of m3
        mov (64 - 8),ct3
        mov 7,lop
        lps
        mov 0,mc3
        mov (64 - 8),ct3

        ;; output: m3
unparse_base10_loop:
                                   mov 0,ct2
        mvi div10_unsigned,pc
        ;;   [X     ]   [Y     ]   [D1     ]
                        mov mc2,y  mov 1,lop     ; mvi imm,pc delay slot (executed twice)
        ;; after function return:
                                   mov all,mc0   ; m0[1] (1234)
                                   mov all,pl
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
        sub             mov alu,a  mov 16,pl

        ;;   a: 5

        ;; convert to vdp2 character
        add             mov alu,a

        ;; store digit in m3
             mov mc0,p  clr a      mov all,mc3   ; m0[1]

        ;;   p: 1234
        ;;   a: 0
        add             mov alu,a  mov 0,ct0

        ;;   a: 1234
        jmp nz,unparse_base10_loop
                                   mov all,mc0   ; jmp delay slot
