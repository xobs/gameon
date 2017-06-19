#ifndef INPUT_SERVER_H_
#define INPUT_SERVER_H_

#include "radio.h"
#include <stdint.h>

struct usb_kbd_report {
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys[6];
} __attribute__((packed));

struct input_server_config {
  uint8_t ready;
  struct usb_kbd_report kbd_report;
};

struct input_server_config *inputServerSetup(KRadioDevice *radio);

#endif /* INPUT_SERVER_H_ */
