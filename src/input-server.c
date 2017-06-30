#include "input-server.h"
#include "radio.h"

static struct input_server_config config;

static const uint8_t key_mapping[16] = {
    0x24, 0x23, 0x21, 0x26, 0x1f, 0x00, 0x00, 0x20,
    0x25, 0x1e, 0x00, 0x00, 0x27, 0x2b, 0x22, 0x00,
};

/*
  10   8   15   1
    5    3    2
  9   4   14   13
 */

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

static void input_request(uint8_t port, uint8_t src, uint8_t dst,
                          uint8_t length, const void *data) {
  (void)port;
  (void)dst;
  (void)length;
  (void)src;

  uint32_t i;

  const uint32_t *state = (const uint32_t *)data;
  int changes           = 0;

  // First pass: Remove any buttons that aren't pressed.
  for (i = 0; i < 6; i++) {
    if (config.kbd_report.keys[i]) {
      uint32_t pin_num = config.kbd_report.keys[i] - 4;
      if (!(*state << pin_num)) {
        changes++;
        config.kbd_report.keys[i] = 0;
      }
    }
  }

  // Next, go through each pin and allocate a report for it.
  for (i = 0; i < 16; i++) {
    if (!(*state & (1 << i)))
      continue;

    uint8_t keycode = key_mapping[i];
    // If we already have the report, don't do anything.
    if ((config.kbd_report.keys[0] == keycode) ||
        (config.kbd_report.keys[1] == keycode) ||
        (config.kbd_report.keys[2] == keycode) ||
        (config.kbd_report.keys[3] == keycode) ||
        (config.kbd_report.keys[4] == keycode) ||
        (config.kbd_report.keys[5] == keycode))
      continue;

    // Look for a spare place to put the key.
    uint32_t j;
    for (j = 0; j < 6; j++) {
      if (config.kbd_report.keys[j] == 0) {
        config.kbd_report.keys[j] = keycode;
        changes++;
        break;
      }
    }
  }

  if (changes)
    config.ready = 1;
}

struct input_server_config *inputServerSetup(KRadioDevice *radio) {
  radioSetHandler(radio, radio_prot_input, input_request);

  return &config;
}
