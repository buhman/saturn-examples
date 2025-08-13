        ;; input data: m0
        ;; input data: m1
        ;; input data: m2

        ;; variables m3:
        loop_0_ix = 0     ; decrementing counter
        loop_1_start = 1

        ;start = 0
        start = 0 ; constant

        ;; initialization
        ;;   [X       ]   [Y       ]   [D1             ]
                                       mov (start + 0),ct0

        ;; initialize multiplier output to 2020
                                       mov 63,ct3
        mvi 2020,mc3                                       ; m3[63] used as temporary
                                       mov 63,ct3
                          mov m3,y     mov 1,rx
        ;; initialize variables
                                       mov loop_0_ix,ct3
                                       mov (64 - start),mc3            ; loop_0_ix+
                                       mov (start + 1),mc3 ; loop_1_start+

        ;; outer loop init
                                       mov loop_inner,top
                                       mov (64 - start),lop          ; (LOP: 64)
                                       mov 1,pl
                                       mov loop_1_start,ct3
solve_loop_0_top:
        ;;   [X       ]   [Y       ]   [D1             ]
        ;; calculate inner loop starting index
        ;; P: 1 ; A: 64
                          mov m3,a     mov m3,ct1          ; loop_1_start
        ;; pre-decrement lop; effectively a jump to loop_inner
        btm                                                ; (LOP: 63)
        add                            mov all,mc3         ; loop_1_start+ (delay slot)

loop_inner:
             mov m0,p     mov m1,a
        add  mov mul,p    mov alu,a
        sub
        jmp z,found
        nop

        btm
        ;; increment ct1 (A discarded)
                          mov mc1,a                        ; (delay slot)

        ;; increment ct0 (A discarded)
                          mov mc0,a

loop_exit_test:
        ;;   [X       ]   [Y       ]   [D1             ]
                                       mov loop_0_ix,ct3
                          mov m3,a     mov 1,pl           ; m3: loop_0_ix
        sub               mov alu,a    mov all,mc3        ; m3: loop_0_ix+
        jmp nz,solve_loop_0_top
                                       mov all,lop        ; (delay slot)

not_found:
        jmp return
                          clr a                           ; (delay slot)
found:
        ;;   [X       ]   [Y            ]   [D1             ]
             mov m1,x     mov m0,y  clr a
             mov mul,p
        add               mov alu,a                       ; (delay slot)
return:
        ;;mov 0,ct0
        ;;mov all,mc0
        ;;endi
        ;;nop
