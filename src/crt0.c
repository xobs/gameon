#include "kl17.h"
#include <stdint.h>

/* Values exported by the linker */
extern uint32_t _eflash;
extern uint32_t _sdtext;
extern uint32_t _edtext;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern uint32_t __main_stack_end__;

struct boot_token {
  uint32_t magic;
  uint32_t boot_count;
};

__attribute__((section("boot_token"))) extern struct boot_token boot_token;

void memcpy32(uint32_t *src, uint32_t *dest, uint32_t count) {
  count /= sizeof(*src);
  while (count--)
    *dest++ = *src++;
}

static void init_crt(void) {
  /* Copy data section to RAM */
  memcpy32(&_eflash, &_sdtext, (uint32_t)&_edtext - (uint32_t)&_sdtext);

  /* Clear BSS */
  uint32_t *dest = &_sbss;
  while (dest < &_ebss)
    *dest++ = 0;
}

extern void __early_init(void);

__attribute__((noreturn)) extern void main(void);

__attribute__((noreturn)) void Reset_Handler(void) {
  init_crt();
  __early_init();

  boot_token.boot_count = 0;
  main();
}
