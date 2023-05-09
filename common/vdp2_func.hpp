static inline void v_blank_out() {
  /*
           v
         _____
    ____|     |____
   */
  while ((vdp2.reg.TVSTAT & TVSTAT__VBLANK) == 0);
  while ((vdp2.reg.TVSTAT & TVSTAT__VBLANK) != 0);
}

static inline void v_blank_in() {
  /*
     v
         _____
    ____|     |____

   */
  while ((vdp2.reg.TVSTAT & TVSTAT__VBLANK) != 0);
  while ((vdp2.reg.TVSTAT & TVSTAT__VBLANK) == 0);
}
