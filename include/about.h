#ifndef ABOUT_H_
#define ABOUT_H_

#include <stdint.h>

struct about_response {
  uint8_t gitver[32];
} __attribute__((packed));

#endif /* ABOUT_H_ */
