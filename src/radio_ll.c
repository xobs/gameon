#include <stdint.h>

#include "palawan.h"
#include "kl17.h"
#include "spi.h"

#define RADIO_REG_DIOMAPPING2 (0x26)
#define RADIO_CLK_DIV1 (0x00)
#define RADIO_CLK_DIV2 (0x01)
#define RADIO_CLK_DIV4 (0x02)
#define RADIO_CLK_DIV8 (0x03)
#define RADIO_CLK_DIV16 (0x04)
#define RADIO_CLK_DIV32 (0x05)
#define RADIO_CLK_RC (0x06)
#define RADIO_CLK_OFF (0x07)

static void radio_reset(void)
{
  GPIOB->PSOR = (1 << 11);
}

static void radio_enable(void)
{
  GPIOB->PCOR = (1 << 11);
}

/**
 * @brief   Read a value from a specified radio register
 *
 * @param[in] addr      radio register to read from
 * @return              value read from said register
 *
 * @notapi
 */
uint8_t radioReadRegister(uint8_t addr)
{
  uint8_t val;

  spiReadStatus();
  spiAssertCs();

  spiXmitByteSync(addr);
  spiRecvByteSync();
  val = spiRecvByteSync();

  spiDeassertCs();

  return val;
}

/**
 * @brief   Write a value to a specified radio register
 *
 * @param[in] addr      radio register to write to
 * @param[in] val       value to write to said register
 *
 * @notapi
 */
void radioWriteRegister(uint8_t addr, uint8_t val)
{

  spiReadStatus();
  spiAssertCs();
  /* Send the address to write */
  spiXmitByteSync(addr | 0x80);

  /* Send the actual value */
  spiXmitByteSync(val);

  (void)spiRecvByteSync();
  spiDeassertCs();
}

static void early_usleep(int usec)
{
  int j, k;

  for (j = 0; j < usec; j++)
    for (k = 0; k < 30; k++)
      asm("");
}

static void early_msleep(int msec)
{
  int i, j, k;

  for (i = 0; i < msec; i++)
    for (j = 0; j < 1000; j++)
      for (k = 0; k < 30; k++)
        asm("");
}

/**
 * @brief   Power cycle the radio
 * @details Put the radio into reset, then wait 100 us.  Bring it out of
 *          reset, then wait 5 ms.  Ish.
 */
int radioPowerCycle(void)
{
  /* Mux the reset GPIO */
  GPIOB->PDDR |= ((uint32_t)1 << 11);
  PORTB->PCR[11] = PORTx_PCRn_MUX(1);

  radio_reset();

  /* Cheesy usleep(100).*/
  early_usleep(100);

  radio_enable();

  /* Cheesy msleep(10).*/
  early_msleep(10);

  return 1;
}

int b8_irqs = 0;
void VectorB8(void)
{
  b8_irqs++;
  asm("bkpt #41");
  radioPoll(0);
  /* Clear all pending interrupts on this port. */
  PORTA->ISFR = 0xFFFFFFFF;
}

/**
 * @brief   Set up mux ports, enable SPI, and set up GPIO.
 * @details The MCU communicates to the radio via SPI and a few GPIO lines.
 *          Set up the pinmux for these GPIO lines, and set up SPI.
 */
static void early_init_radio(void)
{

  /* Enable Reset GPIO and SPI PORT clocks by unblocking
   * PORTC, PORTD, and PORTE.*/
  SIM->SCGC5 |= (SIM_SCGC5_PORTA | SIM_SCGC5_PORTB);

  /* Map Reset to a GPIO, which is connected to PTB11. */
  GPIOB->PDDR |= ((uint32_t)1 << 11);
  GPIOB->PSOR = (1 << 11);
  PORTB->PCR[11] = PORTx_PCRn_MUX(1);

  /* Mux PTA8 as DIO0, to receive packets.
   * Enable the interrupt, and mux it as a slow slew rate.
   */
  PORTA->PCR[8] = PORTx_PCRn_MUX(1) | (1 << 2) | (0xb << 16);
  GPIOA->PDDR &= ~(1 << 8);

  /* Mux PTB2 as DIO1, useful for knowing when the FIFO is empty.
   */
  PORTB->PCR[2] = PORTx_PCRn_MUX(1) | (1 << 2);
  GPIOB->PDDR &= ~(1 << 2);

  /* Unmask PORTA IRQs, so VectorB8() will get called. */
  NVIC_EnableIRQ(PINA_IRQn);

  /* Keep the radio in reset.*/
  radio_reset();

  spiReadStatus();
  spiDeassertCs();
}

/**
 * @brief   Configure the radio to output a given clock frequency
 *
 * @param[in] osc_div   a factor of division, of the form 2^n
 *
 * @notapi
 */

/* This optimization fix causes it to read DIOMAPPING2 three times, and
 * for reasons unknown, this actually causes it to stick.
 * Maybe it's a race condition, I'm not really sure.  But if you don't
 * do the readback afterwards, then the clock gets stuck at 1 MHz rather
 * than 8 MHz, and bad things happen.
 */
#define DIOVAL2_OPTIMIZATION_FIX
static void radio_configure_clko(uint8_t osc_div)
{
#ifdef DIOVAL2_OPTIMIZATION_FIX
  uint8_t dioval2 = radioReadRegister(RADIO_REG_DIOMAPPING2);
  radioWriteRegister(RADIO_REG_DIOMAPPING2, (dioval2 & ~7) | osc_div);
  dioval2 = radioReadRegister(RADIO_REG_DIOMAPPING2);
  radioWriteRegister(RADIO_REG_DIOMAPPING2, (dioval2 & ~7) | osc_div);
  (void)radioReadRegister(RADIO_REG_DIOMAPPING2);
#else
  radioWriteRegister(RADIO_REG_DIOMAPPING2,
                     (radioReadRegister(RADIO_REG_DIOMAPPING2) & ~7) | osc_div);
#endif
}

void radioInit(void)
{
  /*
  switch (palawanModel())
  {
  case palawan_tx:
    break;

  case palawan_rx:
    break;

  default:
    asm("bkpt #0");
    break;
  }
*/
  early_init_radio();
  radioPowerCycle();

  /* 32Mhz/4 = 8 MHz CLKOUT.*/
  //radio_configure_clko(RADIO_CLK_DIV1);
}
