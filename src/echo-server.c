#include "radio.h"

static void echo_request(uint8_t port,
                         uint8_t src,
                         uint8_t dst,
                         uint8_t length,
                         const void *data) {
  (void)port;
  (void)dst;
  (void)length;
  (void)src;
  (void)data;
}

void echoServerSetup(KRadioDevice *radio) {
  radioSetHandler(radio, radio_prot_echo, echo_request);
}
