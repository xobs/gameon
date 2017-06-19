#include "kl17.h"
#include "palawan.h"
#include "radio.h"
#include <stdint.h>

struct dhcp_request {
  uint64_t uid;
  uint8_t addr;
} __attribute__((packed));

static const uint64_t get_uid(void) {
  uint64_t uidh = *((uint32_t *)0x40048058);
  uint64_t uidm = *((uint32_t *)0x4004805c);
  uint64_t uidl = *((uint32_t *)0x40048060);
  uint64_t uid;

  uid = (uidh << 48) | (uidm << 16) | (uidl >> 16);
  return uid;
}

static uint8_t dhcp_requesting;

static void dhcp_response(uint8_t port, uint8_t src, uint8_t dst,
                          uint8_t length, const void *data) {
  (void)port;
  (void)dst;
  (void)length;
  (void)src;
  const struct dhcp_request *req = data;

  /* Client code (e.g. we got a response) */
  if (req->uid == get_uid()) {

    radioSetAddress(radioDevice, req->addr);
    dhcp_requesting = 0;
  }
  FGPIOB->PTOR = (1 << 1);

  return;
}

int dhcpRequestAddress(int timeout_ms) {

  struct dhcp_request request;
  int ms = 0;

  request.uid  = get_uid();
  request.addr = 0;

  dhcp_requesting = 1;
  radioSetHandler(radioDevice, radio_prot_dhcp_response, dhcp_response);
  radioSend(radioDevice, 0xff, radio_prot_dhcp_request, sizeof(request),
            &request);

  while (dhcp_requesting && (ms < timeout_ms)) {
    radioPoll(radioDevice);
    int i;
    for (i = 0; i < 5000; i++)
      asm("nop");
    ms++;
  }

  if (dhcp_requesting)
    ms = -1;

  dhcp_requesting = 0;

  return ms;
}
