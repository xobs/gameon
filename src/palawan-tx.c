#include "kl17.h"
#include "memio.h"

void palawanTxPinSetup(void) {
  PORTA->PCR[3]  = PORTx_PCRn_MUX(1) | PORTx_PCRn_PE | PORTx_PCRn_PS;
  PORTA->PCR[4]  = PORTx_PCRn_MUX(1) | PORTx_PCRn_PE | PORTx_PCRn_PS;
  PORTA->PCR[9]  = PORTx_PCRn_MUX(1) | PORTx_PCRn_PE | PORTx_PCRn_PS;
  PORTA->PCR[10] = PORTx_PCRn_MUX(1) | PORTx_PCRn_PE | PORTx_PCRn_PS;
  PORTA->PCR[11] = PORTx_PCRn_MUX(1) | PORTx_PCRn_PE | PORTx_PCRn_PS;
  PORTA->PCR[12] = PORTx_PCRn_MUX(1) | PORTx_PCRn_PE | PORTx_PCRn_PS;
  PORTA->PCR[13] = PORTx_PCRn_MUX(1) | PORTx_PCRn_PE | PORTx_PCRn_PS;

  PORTB->PCR[3]  = PORTx_PCRn_MUX(1) | PORTx_PCRn_PE | PORTx_PCRn_PS;
  PORTB->PCR[4]  = PORTx_PCRn_MUX(1) | PORTx_PCRn_PE | PORTx_PCRn_PS;
  PORTB->PCR[5]  = PORTx_PCRn_MUX(1) | PORTx_PCRn_PE | PORTx_PCRn_PS;
  PORTB->PCR[6]  = PORTx_PCRn_MUX(1) | PORTx_PCRn_PE | PORTx_PCRn_PS;
  PORTB->PCR[7]  = PORTx_PCRn_MUX(1) | PORTx_PCRn_PE | PORTx_PCRn_PS;
  PORTB->PCR[8]  = PORTx_PCRn_MUX(1) | PORTx_PCRn_PE | PORTx_PCRn_PS;
  PORTB->PCR[9]  = PORTx_PCRn_MUX(1) | PORTx_PCRn_PE | PORTx_PCRn_PS;
  PORTB->PCR[10] = PORTx_PCRn_MUX(1) | PORTx_PCRn_PE | PORTx_PCRn_PS;
  PORTB->PCR[13] = PORTx_PCRn_MUX(1) | PORTx_PCRn_PE | PORTx_PCRn_PS;
}

uint32_t palawanTxReadPins(void) {
  uint32_t porta = ~(FGPIOA->PDIR);
  uint32_t portb = ~(FGPIOB->PDIR);
  uint32_t result;

#define PAD_SHIFT(port, pad, shift) ((!!((port & (1 << pad)))) << shift)
  result = PAD_SHIFT(porta, 3, 0) | PAD_SHIFT(porta, 4, 1) |
           PAD_SHIFT(porta, 9, 2) | PAD_SHIFT(porta, 10, 3) |
           PAD_SHIFT(porta, 11, 4) | PAD_SHIFT(porta, 12, 5) |
           PAD_SHIFT(porta, 13, 6) | PAD_SHIFT(portb, 3, 7) |
           PAD_SHIFT(portb, 4, 8) | PAD_SHIFT(portb, 5, 9) |
           PAD_SHIFT(portb, 6, 10) | PAD_SHIFT(portb, 7, 11) |
           PAD_SHIFT(portb, 8, 12) | PAD_SHIFT(portb, 9, 13) |
           PAD_SHIFT(portb, 10, 14) | PAD_SHIFT(portb, 13, 15);
  return result;
}
