        ;;
        ;; inline function: unparse_base10
        ;;

        ;; argument: m2[0]
        ;; return: base10 + 16 in m3[56:63]

        ;; requires: div10_unsigned

        ;;   [X     ]   [Y     ]   [D1     ]
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
                                   mov all,mc2   ; m2[1] (1234)
                                   mov all,pl
        ;; mod10: multiply by 10:
        sl              mov alu,a
        sl              mov alu,a
        sl              mov alu,a
        add             mov alu,a
        add             mov alu,a  mov 0,ct2     ; restore ct2 after 'mov all,mc2'

        ;;   a: 12340
                        mov mc2,a  mov all,pl

        ;;   a: 12345   m2[0]
        ;;   p: 12340

        ;; mod10: subtract (a - p)
        sub             mov alu,a  mov 16,pl

        ;;   a: 5

        ;; convert to vdp2 character
        add             mov alu,a

        ;; store digit in m3
             mov mc2,p  clr a      mov all,mc3   ; m2[1]

        ;;   p: 1234
        ;;   a: 0
        add             mov alu,a  mov 0,ct2

        ;;   a: 1234
        jmp nz,unparse_base10_loop
                                   mov all,mc2   ; jmp delay slot
