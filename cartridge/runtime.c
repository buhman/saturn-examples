#include <stdint.h>

extern uint32_t __text_link_start __asm("__text_link_start");
extern uint32_t __text_link_end __asm("__text_link_end");
extern uint32_t __text_load_start __asm("__text_load_start");

extern uint32_t __data_link_start __asm("__data_link_start");
extern uint32_t __data_link_end __asm("__data_link_end");
extern uint32_t __data_load_start __asm("__data_load_start");

extern uint32_t __rodata_link_start __asm("__rodata_link_start");
extern uint32_t __rodata_link_end __asm("__rodata_link_end");
extern uint32_t __rodata_load_start __asm("__rodata_load_start");

extern uint32_t __ctors_link_start __asm("__ctors_link_start");
extern uint32_t __ctors_link_end __asm("__ctors_link_end");

extern uint32_t __bss_link_start __asm("__bss_link_start");
extern uint32_t __bss_link_end __asm("__bss_link_end");

void copy(uint32_t * start, const uint32_t * end, uint32_t * load)
__attribute__((section(".text.startup.copy")));

void copy(uint32_t * start, const uint32_t * end, uint32_t * load)
{
  if (start != load) {
    while (start < end) {
      *start++ = *load++;
    }
  }
}

extern void main(void);

typedef void(init_t)(void);

void runtime_init(void)
__attribute__((section(".text.startup.runtime_init")));

void runtime_init(void)
{
  // relocate text (if necessary)
  copy(&__text_link_start, &__text_link_end, &__text_load_start);

  // relocate data (if necessary)
  copy(&__data_link_start, &__data_link_end, &__data_load_start);

  // relocate rodata (if necessary)
  copy(&__rodata_link_start, &__rodata_link_end, &__rodata_load_start);

  uint32_t * start;
  uint32_t * end;

  // clear BSS
  start = &__bss_link_start;
  end = &__bss_link_end;
  while (start < end) {
    *start++ = 0;
  }

  // call ctors
  start = &__ctors_link_start;
  end = &__ctors_link_end;
  while (start < end) {
    ((init_t*)(*start++))();
  }
}
