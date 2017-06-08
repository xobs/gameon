#include <stdint.h>
#include "kl17.h"
#include "spi.h"
#include "palawan.h"
#include "radio.h"

extern void usbStart(void);
void usbProcess(void (*received_data)(void *data, uint32_t size));

static void usb_received_data(void *data, uint32_t bytes)
{
  (void)data;
  (void)bytes;
  FGPIOB->PTOR = (1 << 1);
}

static void radio_process(void)
{
  extern uint8_t radioHasData;
  if (!radioHasData)
    return;

  radioHasData = 0;
  radioPollDefault();
}

static void configure_led(void)
{
  PORTB->PCR[1] = (1 << 8) | (1 << 2);
  FGPIOB->PDDR |= (1 << 1);
  FGPIOB->PSOR = (1 << 1);
}

__attribute__((noreturn)) void main(void)
{
  configure_led();
  spiInit();
  radioInit();

  radioStart(radioDevice);
  if (palawanModel() == palawan_rx) {
    radioSetAddress(radioDevice, 0);
  }

  usbStart();
  radioDumpFifo();
  radioDumpData(1, 1);

  while (1) {
    usbProcess(usb_received_data);
    radio_process();
  }
}
