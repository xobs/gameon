#include "palawan.h"
#include "kl17.h"
#include "memio.h"

void spiAssertCs(void)
{
  GPIOA->PCOR = (1 << 5);
}

void spiDeassertCs(void)
{
  GPIOA->PSOR = (1 << 5);
}

void spiReadStatus(void)
{
  (void)SPI0->S;
}

/**
 * @brief   Send a dummy byte and return the response
 *
 * @return              value read from said register
 *
 * @notapi
 */
uint8_t spiTranscieve(uint8_t byte)
{
  /* Wait for the Tx FIFO to clear up */
  while (!(readb(SPI0_S) & SPIx_S_SPTEF))
    ;

  /* Without this read first, the write is ignored */
  //(void)SPI0->S;

  /* Send the byte, to induce a transfer */
  writeb(byte, SPI0_D);

  /* Wait for the incoming byte to be available */
  while (!(readb(SPI0_S) & SPIx_S_SPRF))
    ;

  /* Return the response */
   return readb(SPI0_D);
}

void spiInit(void)
{
  /* Enable SPI clock.*/
  SIM->SCGC4 |= SIM_SCGC4_SPI0;

  /* Enable GPIO pin control. */
  SIM->SCGC5 |= (SIM_SCGC5_PORTA | SIM_SCGC5_PORTB);

  /* Mux PTA5 as a GPIO, since it's used for Chip Select.*/
  GPIOA->PDDR |= ((uint32_t)1 << 5);
  PORTA->PCR[5] = PORTx_PCRn_MUX(1) | (1 << 2);

  /* Mux PTB0 as SCK */
  PORTB->PCR[0] = PORTx_PCRn_MUX(3) | (1 << 2);

  /* Mux PTA6 as MISO */
  PORTA->PCR[6] = PORTx_PCRn_MUX(3) | (1 << 2);

  /* Mux PTA7 as MOSI */
  PORTA->PCR[7] = PORTx_PCRn_MUX(3) | (1 << 2);

  /* Initialize the SPI peripheral default values.*/
  SPI0->C1 = 0;
  SPI0->C2 = 0;
  SPI0->BR = 0;

  /* Enable SPI system, and run as a Master.*/
  SPI0->C1 |= (SPIx_C1_SPE | SPIx_C1_MSTR);
}
