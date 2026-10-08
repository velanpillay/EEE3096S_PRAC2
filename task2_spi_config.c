/**
  ******************************************************************************
  * @file    task2_spi_config.c
  * @brief   TASK 2 : CONFIGURE THE HARDWARE SPI PERIPHERAL
  ******************************************************************************
  */

#include "prac2a.h"

volatile uint32_t dbg_gpiob_moder      = 0u;
volatile uint32_t dbg_gpiob_afrh       = 0u;
volatile uint32_t dbg_spi_cr1          = 0u;
volatile uint32_t dbg_spi_cr2          = 0u;
volatile uint32_t dbg_spi_sr           = 0u;
volatile uint32_t dbg_sck_hz_predicted = 0u;


void eeprom_spi_init(void)
{
    uint32_t moder;
    uint32_t afrh;
    uint32_t speed;
    uint32_t pupdr;
    uint32_t cr1;
    uint32_t cr2;


    /* ================================================================
     * TODO 2.4
     * Enable GPIOB and SPI2 peripheral clocks.
     *
     * GPIOB is on AHB.
     * SPI2 is on APB1.
     * ================================================================ */

    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    (void)RCC->AHBENR;

    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
    (void)RCC->APB1ENR;


    /* ================================================================
     * TODO 2.5
     * Configure PB12 as manual GPIO chip select.
     *
     * IMPORTANT:
     * Set the output latch HIGH before changing PB12 to output mode.
     * This avoids briefly selecting the EEPROM during configuration.
     * ================================================================ */

    EE_SPI_GPIO->BSRR = EE_CS_MASK;

    moder = EE_SPI_GPIO->MODER;

    /* Clear PB12 mode bits */
    moder &= ~MODER2_MASK(EE_PIN_CS);

    /* 01 = general-purpose output */
    moder |= MODER2(EE_PIN_CS, 1u);

    EE_SPI_GPIO->MODER = moder;

    /* Explicitly select push-pull for CS */
    EE_SPI_GPIO->OTYPER &= ~(1UL << EE_PIN_CS);


    /* ================================================================
     * TODO 2.6
     * Configure:
     *
     * PB13 = SPI2_SCK
     * PB14 = SPI2_MISO
     * PB15 = SPI2_MOSI
     *
     * GPIO mode 10 = alternate-function mode.
     * AF0 connects these pins to SPI2.
     * ================================================================ */

    moder = EE_SPI_GPIO->MODER;

    moder &= ~(MODER2_MASK(EE_PIN_SCK)
             | MODER2_MASK(EE_PIN_MISO)
             | MODER2_MASK(EE_PIN_MOSI));

    moder |= MODER2(EE_PIN_SCK,  2u)
           | MODER2(EE_PIN_MISO, 2u)
           | MODER2(EE_PIN_MOSI, 2u);

    EE_SPI_GPIO->MODER = moder;


    /* PB13-PB15 are in AFRH because they are pins 8..15. */

    afrh = EE_SPI_GPIO->AFR[1];

    afrh &= ~(AFRH4_MASK(EE_PIN_SCK)
            | AFRH4_MASK(EE_PIN_MISO)
            | AFRH4_MASK(EE_PIN_MOSI));

    afrh |= AFRH4(EE_PIN_SCK,  EE_SPI_AF)
          | AFRH4(EE_PIN_MISO, EE_SPI_AF)
          | AFRH4(EE_PIN_MOSI, EE_SPI_AF);

    EE_SPI_GPIO->AFR[1] = afrh;


    /* SCK and MOSI are driven outputs, so use push-pull. */
    EE_SPI_GPIO->OTYPER &= ~((1UL << EE_PIN_SCK)
                           | (1UL << EE_PIN_MOSI));


    /* ================================================================
     * TODO 2.7
     * High output speed on SCK and MOSI.
     * Pull-up on MISO.
     * ================================================================ */

    speed = EE_SPI_GPIO->OSPEEDR;

    speed &= ~(MODER2_MASK(EE_PIN_SCK)
             | MODER2_MASK(EE_PIN_MOSI));

    /* 11 = high speed */
    speed |= MODER2(EE_PIN_SCK,  3u)
           | MODER2(EE_PIN_MOSI, 3u);

    EE_SPI_GPIO->OSPEEDR = speed;


    pupdr = EE_SPI_GPIO->PUPDR;

    pupdr &= ~MODER2_MASK(EE_PIN_MISO);

    /* 01 = pull-up */
    pupdr |= MODER2(EE_PIN_MISO, 1u);

    EE_SPI_GPIO->PUPDR = pupdr;


    /* ================================================================
     * TODO 2.8
     * SPI_CR2
     *
     * DS[3:0] = 0111 -> 8-bit frame
     * FRXTH = 1       -> RXNE set when >= 8 bits received
     * ================================================================ */

    cr2 = EE_SPI->CR2;

    /* Clear DS[3:0] and FRXTH */
    cr2 &= ~((0xFUL << 8) | SPI_CR2_FRXTH);

    /* 0111 = 8-bit data */
    cr2 |= (7UL << 8);

    /* RXNE threshold = 8 bits */
    cr2 |= SPI_CR2_FRXTH;

    EE_SPI->CR2 = cr2;


    /* ================================================================
     * TODO 2.9
     * SPI_CR1
     *
     * MSTR      = 1  -> master
     * BR        = 100 -> divide PCLK by 32
     * CPOL      = 0
     * CPHA      = 0     SPI mode 0
     * LSBFIRST  = 0  -> MSB first
     *
     * SSM = 1 and SSI = 1 because CS is controlled manually using PB12.
     * ================================================================ */

    cr1 = EE_SPI->CR1;

    /* Peripheral must still be disabled while configuring it. */
    cr1 &= ~SPI_CR1_SPE;

    /* Clear the fields we are configuring. */
    cr1 &= ~((7UL << 3)          /* BR[2:0] */
           | SPI_CR1_CPOL
           | SPI_CR1_CPHA
           | SPI_CR1_LSBFIRST
           | SPI_CR1_MSTR
           | SPI_CR1_SSM
           | SPI_CR1_SSI
           | SPI_CR1_RXONLY
           | SPI_CR1_BIDIMODE);

    /* Master mode */
    cr1 |= SPI_CR1_MSTR;

    /* BR = 100b -> PCLK / 32 */
    cr1 |= (EE_SPI_BR << 3);

    /*
     * Software NSS management.
     *
     * SSI = 1 makes the internal NSS level HIGH so the SPI peripheral
     * remains in master mode while PB12 is controlled independently.
     */
    cr1 |= SPI_CR1_SSM | SPI_CR1_SSI;

    EE_SPI->CR1 = cr1;


    /* ---------------------------------------------------------------
     * Task 5 fault case.
     * DO NOT MOVE OR EDIT THIS.
     * --------------------------------------------------------------- */

    task5_fault_hook();


    /* ================================================================
     * TODO 2.10
     * Enable SPI2.
     * ================================================================ */

    EE_SPI->CR1 |= SPI_CR1_SPE;


    /* Debug values supplied by the starter project. */

    dbg_gpiob_moder      = EE_SPI_GPIO->MODER;
    dbg_gpiob_afrh       = EE_SPI_GPIO->AFR[1];
    dbg_spi_cr1          = EE_SPI->CR1;
    dbg_spi_cr2          = EE_SPI->CR2;
    dbg_spi_sr           = EE_SPI->SR;
    dbg_sck_hz_predicted = EE_SCK_HZ_PREDICTED;
}


/* ==========================================================================
 * RUN_TASK 2 - given
 * ========================================================================== */

void task2_setup(void)
{
    task1_gpio_init();
    eeprom_spi_init();
}


void task2_loop(uint32_t now)
{
    task1_gpio_update(now);
}
