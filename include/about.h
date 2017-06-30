#ifndef ABOUT_H_
#define ABOUT_H_

#include <stdint.h>

#define FIRMWARE_SEED 0x84ff3209
struct about_response {
  uint32_t firmware_hash;
  uint8_t gitver[32];
} __attribute__((packed));

#endif /* ABOUT_H_ */
