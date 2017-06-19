#include "about.h"
#include "gitversion.h"
#include "kl17.h"
#include "palawan.h"
#include "radio.h"
#include <string.h>

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

/* Low-Power Timer interrupt */
void VectorB0(void) {
  LPTMR0->CSR = LPTMRx_CSR_TCF | LPTMRx_CSR_TIE | LPTMRx_CSR_TEN;
}

void palawanTxEnableTimer(void) {
  /* Set up the low power timer */
  SIM->SCGC5 |= SIM_SCGC5_LPTMR;

  /* Reset the CSR, in case we're coming out of a warm reset */
  LPTMR0->CSR = 0;

  /* Select LPR */
  LPTMR0->PSR = LPTMRx_PSR_PBYP | LPTMRx_PSR_PCS(1);
  LPTMR0->CMR = 5; /* Approximate poll frequency in ms */
  LPTMR0->CSR = LPTMRx_CSR_TIE | LPTMRx_CSR_TCF;

  LPTMR0->CSR = LPTMRx_CSR_TIE | LPTMRx_CSR_TCF | LPTMRx_CSR_TEN;
  __enable_irq();
  NVIC_EnableIRQ(LPTMR0_IRQn);
}

static uint8_t about_requesting;

static void about_response(uint8_t port, uint8_t src, uint8_t dst,
                           uint8_t length, const void *data) {
  (void)port;
  (void)src;
  (void)dst;
  (void)length;

  const struct about_response *response = data;
  if (memcmp(git_version, response->gitver, strlen(git_version))) {
    boot_token.magic = BOOT_MAGIC;
    asm("bkpt #4");
    NVIC_SystemReset();
  }
  about_requesting = 0;
}

void palawanTxVerifyFirmware(void) {
  radioSetHandler(radioDevice, radio_prot_about_response, about_response);

  uint32_t timeout_ms = 30;

  while (1) {
    uint32_t ms = 0;

    about_requesting = 1;
    radioSend(radioDevice, 0x00, radio_prot_about_request, 0, NULL);
    while (about_requesting && (ms < timeout_ms)) {
      radioPoll(radioDevice);
      int i;
      for (i = 0; i < 5000; i++)
        asm("nop");
      ms++;
    }

    if (!about_requesting)
      break;
  }
}
