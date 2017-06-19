#include "radio.h"

struct usb_kbd_report {
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys[6];
} __attribute__((packed));

enum usb_kbd_modifiers {
  USB_MOD_LEFT_CTRL   = (1 << 0),
  USB_MOD_LEFT_SHIFT  = (1 << 1),
  USB_MOD_LEFT_ALT    = (1 << 2),
  USB_MOD_LEFT_GUI    = (1 << 3),
  USB_MOD_RIGHT_CTRL  = (1 << 4),
  USB_MOD_RIGHT_SHIFT = (1 << 5),
  USB_MOD_RIGHT_ALT   = (1 << 6),
  USB_MOD_RIGHT_GUI   = (1 << 7),
};

static struct usb_kbd_report current_report;

static void input_request(uint8_t port, uint8_t src, uint8_t dst,
                          uint8_t length, const void *data) {
  (void)port;
  (void)dst;
  (void)length;
  (void)src;

  uint32_t i;

  const uint32_t *state = (const uint32_t *)data;

  // First pass: Remove any buttons that aren't pressed.
  for (i = 0; i < 6; i++) {
    if (current_report.keys[i]) {
      uint32_t pin_num = current_report.keys[i] - 4;
      if (!(*state << pin_num))
        current_report.keys[i] = 0;
    }
  }

  // Next, go through each pin and allocate a report for it.
  for (i = 0; i < 16; i++) {
    if (!(*state & (1 << i)))
      continue;

    uint8_t keycode = i + 4;
    // If we already have the report, don't do anything.
    if ((current_report.keys[0] == keycode) ||
        (current_report.keys[1] == keycode) ||
        (current_report.keys[2] == keycode) ||
        (current_report.keys[3] == keycode) ||
        (current_report.keys[4] == keycode) ||
        (current_report.keys[5] == keycode))
      continue;

    // Look for a spare place to put the key.
    uint32_t j;
    for (j = 0; j < 6; j++) {
      if (current_report.keys[j] == 0) {
        current_report.keys[j] = keycode;
        break;
      }
    }
  }

  int usbSend(const void *data, int len);
  usbSend(&current_report, sizeof(current_report));
}

void inputServerSetup(KRadioDevice *radio) {
  radioSetHandler(radio, radio_prot_input, input_request);
}
