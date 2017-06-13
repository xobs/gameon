#include <stdint.h>
#include "radio.h"
#include "kl17.h"

struct dhcp_request {
  uint64_t uid;
  uint8_t addr;
} __attribute__((packed));

struct dhcp_entry {
  uint64_t  uid;
  // This allows us to kick out the oldest value.
  uint32_t  seq;

  uint8_t   addr;
};

static struct dhcp_table {
  struct dhcp_entry entries[10];
  uint32_t seq;
} dhcp_table;

static void dhcp_request(uint8_t port,
                         uint8_t src,
                         uint8_t dst,
                         uint8_t length,
                         const void *data) {
  (void)port;
  (void)dst;
  (void)length;
  (void)src;
  unsigned int i;

  const struct dhcp_request *req = data;
  uint32_t youngest_val = dhcp_table.seq;
  uint32_t youngest_idx = 0;
  struct dhcp_request response;

  // Look through all values for an empty slot, or the youngest value.
  for (i = 0; i < ARRAY_SIZE(dhcp_table.entries); i++) {
    // If the uid matches, it's "free"
    if (dhcp_table.entries[i].uid == req->uid) {
      youngest_idx = i;
      break;
    }
    // IF the uid is null, then it's also free.  Claim it.
    if (!dhcp_table.entries[i].uid) {
      youngest_idx = i;
      break;
    }

    // If this value is less than the current minimum, memorize the address.
    if (dhcp_table.entries[i].seq < youngest_val) {
      youngest_val = dhcp_table.entries[i].seq;
      youngest_idx = i;
    }
  }

  // Allocate the address in the table, and give it the new sequence number.
  dhcp_table.entries[youngest_idx].uid = req->uid;
  dhcp_table.entries[youngest_idx].seq = dhcp_table.seq++;

  // Send a response to the host, allocating the new address.
  // Note: addresses are offset by 2, to reserve the first couple for us.
  response.uid = req->uid;
  response.addr = youngest_idx + 2;
  radioSend(radioDevice, RADIO_BROADCAST_ADDRESS, radio_prot_dhcp_response, sizeof(response), &response);
}

void dhcpServerSetup(KRadioDevice *radio) {
	radioSetHandler(radio, radio_prot_dhcp_request, dhcp_request);
}
