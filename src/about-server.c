#include "about.h"
#include "gitversion.h"
#include "kl17.h"
#include "murmur3.h"
#include "radio.h"
#include <stdint.h>
#include <string.h>

static void about_request(uint8_t port, uint8_t src, uint8_t dst,
                          uint8_t length, const void *data) {
  (void)port;
  (void)dst;
  (void)length;
  (void)data;

  struct about_response response = {};
  strncpy((char *)response.gitver, git_version, sizeof(response.gitver) - 1);
  extern uint32_t __app_start__;
  extern uint32_t __app_end__;
  MurmurHash3_x86_32(&__app_start__,
                     ((uint32_t)&__app_end__) - ((uint32_t)&__app_start__),
                     FIRMWARE_SEED, &response.firmware_hash);

  radioSend(radioDevice, src, radio_prot_about_response, sizeof(response),
            &response);
}

void aboutServerSetup(KRadioDevice *radio) {
  radioSetHandler(radio, radio_prot_about_request, about_request);
}
