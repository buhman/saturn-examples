        .section .text.start

        /* vdp2 */

        /* set TV Mode (see the definition of tvmd_* below) */
        mov.l tvmd,r1
        mov.w tvmd_v,r2
        mov.w tvmd_r,r3
        or r3,r2
        mov.w r2,@r1

        /* turn off all screens (make BACK visible) */
        mov.l bgon,r1
        mov #0,r2
        mov.w r2,@r1

        /* set the Back Screen color mode to "single color" */
        /* set the high bits of the Back Screen Table Address to zero */
        mov.l bktau,r1
        mov.w bktau_v,r2
        mov.w r2,@r1

        /* set the low bits of Back Screen Table Address */
        /* (relative to the beginning of VDP2 VRAM) */
        mov.l bktal,r1
        mov.w bktal_v,r2
        mov.w r2,@r1

        /* write "red" to VDP2 VRAM */
        mov.l vram,r1
        mov #31,r2
        mov.w r2,@r1

loop_forever:
        bra loop_forever
        nop

        /* constants */

        CACHE_THROUGH = 0x20000000
        VDP2_BASE = 0x05E00000

        VRAM = 0x000000

        TVMD = 0x180000
        TVMD__DISP = (1 << 15)
        TVMD__BDCLMD = (1 << 8)
        TVMD__LSMD__NON_INTERLACE = (0b00 << 6)
        TVMD__VRESO__240 = (0b01 << 4)
        TVMD__HRESO__NORMAL_320 = (0b000 << 0)

        BGON = 0x180020


        BKTAU = 0x1800AC
        BKTAU__BKCLMD_SINGLE_COLOR = (0 << 15)

        BKTAL = 0x1800AE

        .align 4
vram:           .long (CACHE_THROUGH | (VDP2_BASE + VRAM))

tvmd:           .long (CACHE_THROUGH | (VDP2_BASE + TVMD))
tvmd_v:         .word (TVMD__DISP | TVMD__BDCLMD | TVMD__LSMD__NON_INTERLACE)
tvmd_r:         .word (TVMD__VRESO__240 | TVMD__HRESO__NORMAL_320)

bktal:          .long (CACHE_THROUGH | (VDP2_BASE + BKTAL))
bktal_v:        .long 32768

bgon:           .long (CACHE_THROUGH | (VDP2_BASE + BGON))

bktau:          .long (CACHE_THROUGH | (VDP2_BASE + BKTAU))
bktau_v:        .word (BKTAU__BKCLMD_SINGLE_COLOR)
