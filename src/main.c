#include "dhcp-server.h"
#include "kl17.h"
#include "palawan.h"
#include "radio.h"
#include "spi.h"
#include <stdint.h>

extern void usbStart(void);
void usbProcess(void (*received_data)(void *data, uint32_t size));
void echoServerSetup(KRadioDevice *radio);
void firmwareServerSetup(KRadioDevice *radio);
void aboutServerSetup(KRadioDevice *radio);

static void received_data(void *data, uint32_t bytes) {
  (void)data;
  (void)bytes;
  FGPIOB->PTOR = (1 << 1);
}

static void configure_led(void) {
  PORTB->PCR[1] = (1 << 8) | (1 << 2);
  FGPIOB->PDDR |= (1 << 1);
  FGPIOB->PSOR = (1 << 1);
}

__attribute__((noreturn)) void palawan_main(void) {
  configure_led();
  spiInit();
  radioInit();

  radioStart(radioDevice);
  switch (palawanModel()) {
  case palawan_rx:
    radioSetAddress(radioDevice, 0);
    dhcpServerSetup(radioDevice);
    echoServerSetup(radioDevice);
    aboutServerSetup(radioDevice);
    firmwareServerSetup(radioDevice);
    usbStart();

    while (1) {
      extern uint8_t packetAvailable;
      usbProcess(received_data);
      if (packetAvailable) {
        packetAvailable = 0;
        radioPoll(radioDevice);
      }
    }
  case palawan_tx:
    break;
  default:
    asm("bkpt #63");
  }
  while (1)
    ;
}
