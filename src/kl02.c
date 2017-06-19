#include "kl17.h"
#include "palawan.h"

#define PAIR_CFG_ADC_NUM (23)

#define PALAWAN_CFG_RESISTANCE_THRESH 64
#define PALAWAN_RX_VALUE 65536
#define PALAWAN_RX_PAIR_VALUE 0

#define PALAWAN_TX_VALUE_1 9906
#define PALAWAN_TX_VALUE_1_PAIR 1028
#define PALAWAN_TX_VALUE_2 20547
#define PALAWAN_TX_VALUE_2_PAIR 2582
#define PALAWAN_TX_VALUE_3 32787
#define PALAWAN_TX_VALUE_3_PAIR 5442
#define PALAWAN_TX_VALUE_4 65483
#define PALAWAN_TX_VALUE_4_PAIR 65127

#define KINETIS_MCG_FLL_DMX32 1   /* Fine-tune for 32.768 kHz */
#define KINETIS_MCG_FLL_DRS 1     /* 1464x FLL factor */
#define KINETIS_MCG_FLL_OUTDIV1 1 /* Divide 48 MHz FLL by 1 => 48 MHz */
#define KINETIS_MCG_FLL_OUTDIV4 2 /* Divide OUTDIV1 output by 2 => 24 MHz */
#define KINETIS_SYSCLK_FREQUENCY 47972352UL /* 32.768 kHz * 1464 (~48 MHz) */

static enum palawan_model _model;

/**
 * @brief   KL2x clocks and PLL initialization.
 * @note    All the involved constants come from the file @p board.h.
 * @note    This function should be invoked just after the system reset.
 *
 * @special
 */
void kl02_clk_init(void) {
  /* Disable COP watchdog */
  SIM->COPC = 0;

  /* Enable PORTA and PORGB */
  SIM->SCGC5 |= SIM_SCGC5_PORTA | SIM_SCGC5_PORTB;

  /* Palawan Rx is the only board with a crystal */
  if (palawanModel() != palawan_rx)
    return;

  /* --- MCG mode: FEI (default out of reset) ---
     f_MCGOUTCLK = f_int * F
     F is the FLL factor selected by C4[DRST_DRS] and C4[DMX32] bits.
     Typical f_MCGOUTCLK = 21 MHz immediately after reset.
     C4[DMX32]=0 and C4[DRST_DRS]=00  =>  FLL factor=640.
     C3[SCTRIM] and C4[SCFTRIM] factory trim values apply to f_int. */

  /* System oscillator drives 32 kHz clock (OSC32KSEL=0) */
  //  SIM->SOPT1 &= ~SIM_SOPT1_OSC32KSEL_MASK;

  /*
   * FLL Enabled External (FEE) MCG Mode
   * 24 MHz core, 12 MHz bus - using 32.768 kHz crystal with FLL.
   * f_MCGOUTCLK = (f_ext / FLL_R) * F
   *             = (32.768 kHz ) *
   *  FLL_R is the reference divider selected by C1[FRDIV]
   *  F is the FLL factor selected by C4[DRST_DRS] and C4[DMX32].
   *
   * Then the core/system and bus/flash clocks are divided:
   *   f_SYS = f_MCGOUTCLK / OUTDIV1 = 48 MHz / 1 = 48 MHz
   *   f_BUS = f_MCGOUTCLK / OUTDIV1 / OUTDIV4 =  MHz / 4 = 24 MHz
   */

  SIM->SOPT2 = SIM_SOPT2_TPMSRC(1); /* MCGFLLCLK clock or MCGPLLCLK/2 */
                                    /* PLLFLLSEL=0 -> MCGFLLCLK */

  /* The MCGOUTCLK is divided by OUTDIV1 and OUTDIV4:
   * OUTDIV1 (divider for core/system and bus/flash clock)
   * OUTDIV4 (additional divider for bus/flash clock) */
  SIM->CLKDIV1 = SIM_CLKDIV1_OUTDIV1(KINETIS_MCG_FLL_OUTDIV1 - 1) |
                 SIM_CLKDIV1_OUTDIV4(KINETIS_MCG_FLL_OUTDIV4 - 1);

  /* EXTAL0 and XTAL0 */
  //  PORTA->PCR[18] &= ~0x01000700; /* Set PA18 to analog (default) */  //
  //  defaults should already be good
  //  PORTA->PCR[19] &= ~0x01000700; /* Set PA19 to analog (default) */

  OSC0->CR = 0xC;

  /* From KL25P80M48SF0RM section 24.5.1.1 "Initializing the MCG". */
  /* To change from FEI mode to FEE mode: */
  /* (1) Select the external clock source in C2 register.
         Use low-power OSC mode (HGO0=0) which enables internal feedback
         resistor, for 32.768 kHz crystal configuration.  */
  MCG->C2 = MCG_C2_RANGE0(0) | /* low frequency range (<= 40 kHz) */
            MCG_C2_EREFS0;     /* external reference (using a crystal) */
  /* (2) Write to C1 to select the clock mode. */
  MCG->C1 = /* Clear the IREFS bit to switch to the external reference. */
      MCG_C1_CLKS_FLLPLL | /* Use FLL for system clock, MCGCLKOUT. */
      MCG_C1_FRDIV(0);     /* Don't divide 32kHz ERCLK FLL reference. */
  MCG->C6 = 0;             /* PLLS=0: Select FLL as MCG source, not PLL */

  /* Loop until S[OSCINIT0] is 1, indicating the
     crystal selected by C2[EREFS0] has been initialized. */
  while ((MCG->S & MCG_S_OSCINIT0) == 0)
    ;
  /* Loop until S[IREFST] is 0, indicating the
     external reference is the current reference clock source. */
  while ((MCG->S & MCG_S_IREFST) != 0)
    ; /* Wait until external reference clock is FLL reference. */
  /* (1)(e) Loop until S[CLKST] indicates FLL feeds MCGOUTCLK. */
  while ((MCG->S & MCG_S_CLKST_MASK) != MCG_S_CLKST_FLL)
    ; /* Wait until FLL has been selected. */

  /* --- MCG mode: FEE --- */
  /* Set frequency range for DCO output (MCGFLLCLK). */
  MCG->C4 = (KINETIS_MCG_FLL_DMX32 ? MCG_C4_DMX32 : 0) |
            MCG_C4_DRST_DRS(KINETIS_MCG_FLL_DRS);

  /* Wait for the FLL lock time; t[fll_acquire][max] = 1 ms */
  /* TODO - not implemented - is it required? Freescale example code
     seems to omit it. */
}

enum palawan_model palawanModel(void) {
  /* The strapping resistors were wired up to a pin that can't do ADC */
  return palawan_rx;
#if 0
  /* Sample the strapping resistors, to determine model type */
  if ((_model != palawan_tx) && (_model != palawan_rx)) {
    uint16_t gain;
    int resistance;
    int resistance_min;
    int resistance_max;

    /* Ungate PORTE, where the ADC we're using lives */
    SIM->SCGC5 |= SIM_SCGC5_PORTB;

    /* Configure PTE30 to be an input pad */
    PORTB->PCR[12] = PORTx_PCRn_MUX(0);

    /* Ungate the ADC */
    SIM->SCGC6 |= SIM_SCGC6_ADC0;

    /* Quick-and-dirty calibration */
    ADC0->CFG1 =  ADCx_CFG1_ADIV(ADCx_CFG1_ADIV_DIV_8) |
                  ADCx_CFG1_ADICLK(ADCx_CFG1_ADIVCLK_BUS_CLOCK_DIV_2);

    /* Software trigger, no DMA */
    ADC0->SC2 = 0;

    /* Enable hardware averaging over 32 samples, and run calibration */
    ADC0->SC3 = ADCx_SC3_AVGE |
                ADCx_SC3_AVGS(ADCx_SC3_AVGS_AVERAGE_32_SAMPLES) |
                ADCx_SC3_CAL;

    /* Wait for calibration to finish */
    while (!(ADC0->SC1A & ADCx_SC1n_COCO))
      ;

    /* Adjust gain according to reference manual */

    gain = ((ADC0->CLP0 + ADC0->CLP1 + ADC0->CLP2 +
             ADC0->CLP3 + ADC0->CLP4 + ADC0->CLPS) / 2) | 0x8000;
    ADC0->PG = gain;

    gain = ((ADC0->CLM0 + ADC0->CLM1 + ADC0->CLM2 +
             ADC0->CLM3 + ADC0->CLM4 + ADC0->CLMS) / 2) | 0x8000;
    ADC0->MG = gain;

    /* Reset Rn */
    (void)ADC0->RA;

    /* Configure for 16-bit conversion */
    ADC0->CFG1 =  ADCx_CFG1_ADIV(ADCx_CFG1_ADIV_DIV_8) |
                  ADCx_CFG1_ADICLK(ADCx_CFG1_ADIVCLK_BUS_CLOCK_DIV_2) |
                  ADCx_CFG1_MODE(ADCx_CFG1_MODE_16_BITS);

    /* Perform the sample read, averaging over 32 samples */
    ADC0->SC3 = ADCx_SC3_AVGE |
                ADCx_SC3_AVGS(ADCx_SC3_AVGS_AVERAGE_32_SAMPLES);

    /* Begin the read */
    ADC0->SC1A = ADCx_SC1n_ADCH(PAIR_CFG_ADC_NUM);

    /* Wait for sample to finish */
    while (!(ADC0->SC1A & ADCx_SC1n_COCO))
      ;

    resistance = ADC0->RA;

    /* Figure out which threshold it falls into */
    resistance_min = resistance - (PALAWAN_CFG_RESISTANCE_THRESH / 2);
    resistance_max = resistance + (PALAWAN_CFG_RESISTANCE_THRESH / 2);

    if ((resistance_min < PALAWAN_RX_VALUE)
     && (resistance_max > PALAWAN_RX_VALUE))
      _model = palawan_rx;
    else if ((resistance_min < PALAWAN_RX_PAIR_VALUE)
          && (resistance_max > PALAWAN_RX_PAIR_VALUE))
      _model = palawan_rx;

    else if ((resistance_min < PALAWAN_TX_VALUE_1)
          && (resistance_max > PALAWAN_TX_VALUE_1))
      _model = palawan_tx;

    else if ((resistance_min < PALAWAN_TX_VALUE_2)
          && (resistance_max > PALAWAN_TX_VALUE_2))
      _model = palawan_tx;

    else if ((resistance_min < PALAWAN_TX_VALUE_3)
          && (resistance_max > PALAWAN_TX_VALUE_3))
      _model = palawan_tx;

    else if ((resistance_min < PALAWAN_TX_VALUE_4)
          && (resistance_max > PALAWAN_TX_VALUE_4))
      _model = palawan_tx;

    else if ((resistance_min < PALAWAN_TX_VALUE_1_PAIR)
          && (resistance_max > PALAWAN_TX_VALUE_1_PAIR))
      _model = palawan_tx;

    else if ((resistance_min < PALAWAN_TX_VALUE_2_PAIR)
          && (resistance_max > PALAWAN_TX_VALUE_2_PAIR))
      _model = palawan_tx;

    else if ((resistance_min < PALAWAN_TX_VALUE_3_PAIR)
          && (resistance_max > PALAWAN_TX_VALUE_3_PAIR))
      _model = palawan_tx;

    else if ((resistance_min < PALAWAN_TX_VALUE_4_PAIR)
          && (resistance_max > PALAWAN_TX_VALUE_4_PAIR))
      _model = palawan_tx;
  }

    return _model;
#endif
}

/**
 * @brief   Early initialization code.
 * @details This initialization must be performed just after stack setup
 *          and before any other initialization.
 */
void __early_init(void) {
  radioPowerCycle();
  kl02_clk_init();
}
