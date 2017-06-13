#include <string.h>
#include "radio.h"
#include "murmur3.h"

struct firmware_request {
  uint32_t type;
  uint32_t offset;
  uint32_t length;
} __attribute__((packed));

struct firmware_murmur_response {
  uint32_t type;
  uint32_t payload_size;
  uint32_t offset;
  uint8_t payload[32];
  uint32_t murmur_sum;
} __attribute__((packed));

#define MURMUR_REQUEST  0x5592493f
#define MURMUR_RESPONSE 0x239b23a9
#define MURMUR_SEED     0xb62a238c

static void firmware_request(uint8_t port,
                             uint8_t src,
                             uint8_t dst,
                             uint8_t length,
                             const void *data) {
  (void)port;
  (void)dst;
  (void)length;
  const struct firmware_request *req = data;
  struct firmware_murmur_response response = {};

  // Only accept MURMUR_REQUEST packets
  if (req->type != MURMUR_REQUEST)
    return;

  // Offsets must be word-aligned
  if (req->offset & 3)
    return;

  uint32_t bytes_to_copy = req->length;
  if (bytes_to_copy > sizeof(response.payload))
    bytes_to_copy = sizeof(response.payload);

  memcpy(response.payload, (uint32_t *)req->offset, bytes_to_copy);

  response.type = MURMUR_RESPONSE;
  response.payload_size = bytes_to_copy;
  response.offset = req->offset;  

  MurmurHash3_x86_32(&response, sizeof(response) - 4, MURMUR_SEED, &response.murmur_sum);

  radioSend(radioDevice, src, radio_prot_firmware_data,
            sizeof(response), &response);
}

void firmwareServerSetup(KRadioDevice *radio) {
	radioSetHandler(radio, radio_prot_request_firmware, firmware_request);
}
