#include <stdint.h>
#include "kl17.h"

extern void usbStart(void);
void usbProcess(void (*received_data)(void *data, uint32_t size));

static void received_data(void *data, uint32_t bytes)
{
  (void)data;
  (void)bytes;
}

__attribute__((noreturn)) int main(void)
{

  PORTB->PCR[1] = (1 << 8) | (1 << 2);
  FGPIOB->PDDR |= (1 << 1);
  FGPIOB->PSOR = (1 << 1);
  /*
  while (1) {
    FGPIOB->PTOR = (1 << 1);

    int ms;
    for (ms = 0; ms < 500; ms++) {
      int i;
      for (i = 0; i < 100; i++) {
        int j;
        for (j = 0; j < 77; j++) {
          asm("");
        }
      }
    }
  }
  */
  usbStart();

  while (1)
    usbProcess(received_data);
}
