/**
  ******************************************************************************
  * @file    task3_spi_transfer.c
  * @brief   TASK 3 : SEND ONE BYTE AND PROVE THE WAVEFORM
  ******************************************************************************
  */

#include "prac2a.h"


/* ==========================================================================
 * Chip select
 * ========================================================================== */

void eeprom_cs_low(void)
{
    EE_SPI_GPIO->BRR = EE_CS_MASK;
}


void eeprom_cs_high(void)
{
    /*
     * TODO 3.3
     *
     * Wait until the transmit buffer is empty, then wait until the SPI
     * peripheral is no longer busy before raising CS.
     *
     * TXE = 1  -> transmit buffer empty
     * BSY = 0  -> final bit has actually left the shift register
     */

    while ((EE_SPI->SR & SPI_SR_TXE) == 0u)
    {
    }

    while ((EE_SPI->SR & SPI_SR_BSY) != 0u)
    {
    }

    EE_SPI_GPIO->BSRR = EE_CS_MASK;
}


/* ==========================================================================
 * TASK 3
 * ========================================================================== */

uint8_t spi_transfer(uint8_t tx)
{
    uint8_t rx;

    /*
     * TODO 3.4
     *
     * Wait until the transmit buffer has room.
     */
    while ((EE_SPI->SR & SPI_SR_TXE) == 0u)
    {
    }


    /*
     * TODO 3.5
     *
     * IMPORTANT:
     * Use an 8-bit write to SPI_DR.
     *
     * If we wrote to DR using its normal 16-bit declaration, the peripheral
     * could transmit a 16-bit frame / produce 16 clocks instead of the
     * required 8 clocks.
     */
    *(volatile uint8_t *)&EE_SPI->DR = tx;


    /*
     * TODO 3.6
     *
     * SPI is full duplex:
     *
     * every transmitted byte also clocks one byte into the receive side.
     *
     * Wait for RXNE, then read the received byte.
     */
    while ((EE_SPI->SR & SPI_SR_RXNE) == 0u)
    {
    }

    rx = *(volatile uint8_t *)&EE_SPI->DR;

    return rx;
}


/* ==========================================================================
 * RUN_TASK 3 - given
 *
 * Sends task3_test_byte every 100 ms with CS held HIGH, so the EEPROM ignores
 * it and your oscilloscope has a repeating burst to trigger on.
 *
 * task3_test_byte can be changed from Live Expressions without a rebuild.
 * ========================================================================== */

#define TASK3_TX_PERIOD_MS  100u

volatile uint8_t  task3_test_byte = (uint8_t)TEST_BYTE_B;
volatile uint8_t  task3_rx_byte   = 0u;
volatile uint32_t task3_tx_count  = 0u;

static uint32_t t3_last = 0u;


void task3_setup(void)
{
    task1_gpio_init();
    eeprom_spi_init();
}


void task3_loop(uint32_t now)
{
    task1_gpio_update(now);

    if ((uint32_t)(now - t3_last) < TASK3_TX_PERIOD_MS)
    {
        return;
    }

    t3_last = now;

    task3_rx_byte = spi_transfer(task3_test_byte);
    task3_tx_count++;
}
