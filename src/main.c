#include <stdint.h>
#include "kl17.h"
#include "spi.h"
#include "palawan.h"
#include "radio.h"
#include "dhcp-server.h"

extern void usbStart(void);
void usbProcess(void (*received_data)(void *data, uint32_t size));

static void received_data(void *data, uint32_t bytes)
{
  (void)data;
  (void)bytes;
  FGPIOB->PTOR = (1 << 1);
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
    dhcpServerSetup(radioDevice);
  }

  usbStart();
  radioDumpFifo();
  radioDumpData(1, 1);

  while (1) {
    extern uint8_t packetAvailable;
    usbProcess(received_data);
    if (packetAvailable) {
      packetAvailable = 0;
      radioPoll(radioDevice);
    }
  #if 0
    else {
      static uint8_t bfr[32];
      static int loops = 0;
      unsigned int i;
      for (i = 0; i < sizeof(bfr); i+=2) {
        bfr[i] = 0x55;
        bfr[i+1] = 0xaa;
      }
      loops++;
      static const char hex_digits[] = "0123456789abcdef";
      bfr[0] = hex_digits[(loops>>12) & 0xf];
      bfr[1] = hex_digits[(loops>>8) & 0xf];
      bfr[2] = hex_digits[(loops>>4) & 0xf];
      bfr[3] = hex_digits[(loops>>0) & 0xf];
      bfr[4] = ' ';
      bfr[31] = '\0';
      radioSend(radioDevice, 0xff, radio_prot_echo, sizeof(bfr), bfr);
    }
#endif
  }

}
