#ifndef __SPI_H__
#define __SPI_H__

void spiAssertCs(void);
void spiDeassertCs(void);
void spiReadStatus(void);

/**
 * @brief   Send a byte byte and return the response
 *
 * @return              value read from said register
 *
 * @notapi
 */
uint8_t spiTranscieve(uint8_t byte);

void spiInit(void);

#endif /*__SPI_H__*/
