#ifndef PALAWAN_H_
#define PALAWAN_H_

#include <stdint.h>

// This describes the structure that allows the OS to communicate
// with the bootloader.  It keeps track of how many times we've
// tried booting, as well as a magic value that tells us to enter
// the bootloader instead of booting the app.
// It also keeps track of the board model.
struct boot_token {
  uint32_t magic;
  uint8_t boot_count;
  uint8_t board_model;
  uint16_t reserved;
} __attribute__((packed));
__attribute__((section("boot_token"))) extern struct boot_token boot_token;
#define BOOT_MAGIC 0x74624346

enum palawan_model {
  palawan_unknown = 0,
  palawan_tx      = 1,
  palawan_rx      = 2,
};

enum palawan_model palawanModel(void);

#endif /* PALAWAN_H_ */
