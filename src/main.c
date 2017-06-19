#include "dhcp-server.h"
#include "input-server.h"
#include "kl17.h"
#include "palawan.h"
#include "radio.h"
#include "spi.h"
#include <stdint.h>

void usbStart(void);
void usbProcess(void (*received_data)(void *data, uint32_t size));
int usbSend(const void *data, int len);

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

static void palawanRxMain(void) {
  struct input_server_config *input_config;

  radioSetAddress(radioDevice, 0);

  input_config = inputServerSetup(radioDevice);
  dhcpServerSetup(radioDevice);
  firmwareServerSetup(radioDevice);
  echoServerSetup(radioDevice);
  aboutServerSetup(radioDevice);

  usbStart();

  while (1) {
    extern uint8_t packetAvailable;
    usbProcess(received_data);
    if (packetAvailable) {
      packetAvailable = 0;
      radioPoll(radioDevice);
    }
    if (input_config->ready) {
      if (usbSend(&input_config->kbd_report,
                  sizeof(input_config->kbd_report)) >= 0) {
        input_config->ready = 0;
      }
    }
  }
}

static void palawanTxMain(void) {

  uint32_t last_pin_state    = 0;
  uint32_t pin_unchanged_for = 0;

  palawanTxPinSetup();

  // First, make sure we have an address
  while (dhcpRequestAddress(200) < 0)
    ;

  // Next, enter a loop looking for changes
  while (1) {
    uint32_t pin_state = palawanTxReadPins();
    if (pin_state != last_pin_state) {
      pin_unchanged_for++;

      // Debounce filter.  Only send if it's settled for a few attempts.
      if ((pin_unchanged_for == 10) || !(pin_unchanged_for & 0xff)) {
        radioSend(radioDevice, 0, radio_prot_input, sizeof(pin_state),
                  &pin_state);
        last_pin_state = pin_state;
      }
    } else
      pin_unchanged_for = 0;
  }
}

__attribute__((noreturn)) void palawan_main(void) {
  configure_led();
  spiInit();
  radioInit();

  radioStart(radioDevice);
  switch (palawanModel()) {
  case palawan_rx:
    palawanRxMain();
    break;

  case palawan_tx:
    palawanTxMain();
    break;

  default:
    asm("bkpt #63");
  }

  while (1)
    ;
}
