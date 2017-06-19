#include "kl17.h"
#include "radio.h"
#include <stdint.h>
#include <string.h>

struct about_response {
  uint8_t gitver[16];
} __attribute__((packed));

static void about_request(uint8_t port, uint8_t src, uint8_t dst,
                          uint8_t length, const void *data) {
  (void)port;
  (void)dst;
  (void)length;
  (void)data;

  struct about_response response = {};
  strncpy((char *)response.gitver, VERSION, sizeof(response.gitver) - 1);

  radioSend(radioDevice, src, radio_prot_about_response, sizeof(response),
            &response);
}

void aboutServerSetup(KRadioDevice *radio) {
  radioSetHandler(radio, radio_prot_about_request, about_request);
}
