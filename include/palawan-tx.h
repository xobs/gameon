#ifndef PALAWAN_TX_H
#define PALAWAN_TX_H

#include <stdint.h>

uint32_t palawanTxReadPins(void);
void palawanTxPinSetup(void);
void palawanTxEnableTimer(void);
void palawanTxVerifyFirmware(void);

#endif /* PALAWAN_TX_H */
