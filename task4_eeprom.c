/**
  ******************************************************************************
  * @file    task4_eeprom.c
  * @brief   TASK 4 : EEPROM DRIVER
  *
  * Build the driver on spi_transfer() - no pre-written EEPROM library.
  * Command values, transaction order and status bits come from the EEPROM
  * datasheet; the address format for this board is in board_config.h.
  *
  * Write completion must be decided from the EEPROM's status register. A fixed
  * delay is not acceptable as proof that a write finished. Blocking while you
  * poll is allowed in Task 4.
  ******************************************************************************
  */

#include "prac2a.h"

volatile uint16_t eeprom_test_addr        = (uint16_t)EEPROM_ADDR_A;
volatile uint8_t  eeprom_test_byte        = (uint8_t)TEST_BYTE_B;

volatile uint8_t  eeprom_status_before    = 0u;
volatile uint8_t  eeprom_status_after     = 0u;
volatile uint8_t  eeprom_read_value       = 0u;
volatile uint8_t  eeprom_verify_ok        = 0u;
volatile uint32_t eeprom_write_wait_ms    = 0u;
volatile uint32_t eeprom_timeout_count    = 0u;


/* ==========================================================================
 * Driver
 * ========================================================================== */

uint8_t eeprom_read_status(void)
{
    uint8_t status;

    /*
     * Pull CS LOW to select EEPROM.
     */
    eeprom_cs_low();

    /*
     * Send Read Status Register command.
     */
    spi_transfer(EEPROM_CMD_RDSR);

    /*
     * SPI is full duplex.
     *
     * We must transmit a dummy byte so that the SPI master generates
     * another 8 clock pulses. While those clocks occur, the EEPROM
     * sends its status register value back on MISO.
     */
    status = spi_transfer(0xFFu);

    /*
     * End transaction.
     */
    eeprom_cs_high();

    return status;
}


void eeprom_write_enable(void)
{
    /*
     * WREN must be its own complete transaction.
     *
     * The write-enable latch is set after the WREN command transaction
     * finishes, so CS must return HIGH before the WRITE transaction starts.
     */
    eeprom_cs_low();

    spi_transfer(EEPROM_CMD_WREN);

    eeprom_cs_high();
}


void eeprom_write_byte(uint16_t address, uint8_t value)
{
    uint32_t start;
    int32_t i;

    /*
     * Prevent access outside the fitted EEPROM.
     */
    if (address >= EEPROM_SIZE_BYTES)
    {
        return;
    }

    /*
     * -------------------------------------------------------------
     * STEP 1: Enable writing
     * -------------------------------------------------------------
     */
    eeprom_write_enable();


    /*
     * -------------------------------------------------------------
     * STEP 2: Perform WRITE transaction
     * -------------------------------------------------------------
     *
     * Transaction:
     *
     * CS LOW
     * WRITE command
     * address MSB
     * address LSB
     * data
     * CS HIGH
     *
     * The board configuration determines EEPROM_ADDR_BYTES.
     */
    eeprom_cs_low();

    spi_transfer(EEPROM_CMD_WRITE);


    /*
     * Send address most-significant byte first.
     *
     * For EEPROM_ADDR_BYTES = 2:
     *
     * first:
     *      address >> 8
     *
     * second:
     *      address >> 0
     */
    for (i = ((int32_t)EEPROM_ADDR_BYTES - 1); i >= 0; i--)
    {
        spi_transfer(
            (uint8_t)((address >> (8u * (uint32_t)i)) & 0xFFu)
        );
    }


    /*
     * Send byte to store.
     */
    spi_transfer(value);


    /*
     * CS rising edge completes the SPI transaction and starts the
     * EEPROM's internal programming operation.
     */
    eeprom_cs_high();


    /*
     * -------------------------------------------------------------
     * STEP 3: Poll the status register until EEPROM finishes
     * -------------------------------------------------------------
     *
     * Task 4 allows blocking polling.
     *
     * RDY bit:
     *
     * 1 = EEPROM busy performing internal write
     * 0 = EEPROM ready
     *
     * EEPROM_WRITE_TIMEOUT_MS prevents a disconnected/faulty EEPROM
     * from hanging the program forever.
     */
    start = HAL_GetTick();

    while ((eeprom_read_status() & EEPROM_SR_RDY) != 0u)
    {
        if ((uint32_t)(HAL_GetTick() - start) >= EEPROM_WRITE_TIMEOUT_MS)
        {
            eeprom_timeout_count++;
            break;
        }
    }


    /*
     * Record how long the write operation took.
     */
    eeprom_write_wait_ms =
        (uint32_t)(HAL_GetTick() - start);
}


uint8_t eeprom_read_byte(uint16_t address)
{
    uint8_t value;
    int32_t i;

    /*
     * Prevent invalid address access.
     */
    if (address >= EEPROM_SIZE_BYTES)
    {
        return 0u;
    }


    /*
     * -------------------------------------------------------------
     * READ transaction
     * -------------------------------------------------------------
     *
     * CS LOW
     * READ command
     * address MSB
     * address LSB
     * dummy byte / receive EEPROM data
     * CS HIGH
     */
    eeprom_cs_low();


    /*
     * Send READ instruction.
     */
    spi_transfer(EEPROM_CMD_READ);


    /*
     * Send address MSB first.
     */
    for (i = ((int32_t)EEPROM_ADDR_BYTES - 1); i >= 0; i--)
    {
        spi_transfer(
            (uint8_t)((address >> (8u * (uint32_t)i)) & 0xFFu)
        );
    }


    /*
     * Send dummy byte to generate 8 SCK pulses.
     *
     * During those clocks the EEPROM transmits the stored data byte
     * on MISO.
     */
    value = spi_transfer(0xFFu);


    /*
     * End READ transaction.
     */
    eeprom_cs_high();


    return value;
}


/* ==========================================================================
 * LEDs
 * ========================================================================== */

void leds_write_byte(uint8_t v)
{
    uint32_t set_mask;
    uint32_t reset_mask;

    /*
     * PB0..PB7 correspond directly to bits 0..7 of v.
     *
     * GPIOB->BSRR lets us SET and RESET GPIO pins in one write without
     * modifying PB10..PB15.
     *
     * BSRR:
     *
     * bits 0..15   = set corresponding GPIO output
     * bits 16..31  = reset corresponding GPIO output
     */

    set_mask =
        ((uint32_t)v & 0xFFu);

    reset_mask =
        (((uint32_t)(~v) & 0xFFu) << 16u);


    GPIOB->BSRR =
        set_mask |
        reset_mask;
}


/* ==========================================================================
 * High-level paths
 * ========================================================================== */

void eeprom_read_only_path(void)
{
    /*
     * Given.
     *
     * Reads EEPROM without writing anything.
     *
     * This allows the demonstrator to:
     *
     * write value
     * reset / power cycle
     * read same value again
     *
     * demonstrating EEPROM persistence.
     */

    eeprom_status_before = eeprom_read_status();

    eeprom_read_value =
        eeprom_read_byte(eeprom_test_addr);

    eeprom_verify_ok =
        (eeprom_read_value == eeprom_test_byte)
        ? 1u
        : 0u;

    leds_write_byte(eeprom_read_value);

    status_leds_show(
        eeprom_verify_ok
        ? STATUS_PASS
        : STATUS_FAIL
    );
}


void eeprom_write_verify_path(void)
{
    /*
     * -------------------------------------------------------------
     * STEP 1
     * Read EEPROM status before the operation.
     * -------------------------------------------------------------
     */
    eeprom_status_before =
        eeprom_read_status();


    /*
     * -------------------------------------------------------------
     * STEP 2
     * Write group test byte to group EEPROM address.
     *
     * eeprom_write_byte() internally:
     *
     * WREN
     * WRITE
     * address
     * data
     * status polling until EEPROM ready
     * -------------------------------------------------------------
     */
    eeprom_write_byte(
        eeprom_test_addr,
        eeprom_test_byte
    );


    /*
     * -------------------------------------------------------------
     * STEP 3
     * Read EEPROM status again after write completion.
     * -------------------------------------------------------------
     */
    eeprom_status_after =
        eeprom_read_status();


    /*
     * -------------------------------------------------------------
     * STEP 4
     * Read the byte back from EEPROM.
     * -------------------------------------------------------------
     */
    eeprom_read_value =
        eeprom_read_byte(eeprom_test_addr);


    /*
     * -------------------------------------------------------------
     * STEP 5
     * Verify returned byte matches the byte written.
     * -------------------------------------------------------------
     */
    eeprom_verify_ok =
        (eeprom_read_value == eeprom_test_byte)
        ? 1u
        : 0u;


    /*
     * -------------------------------------------------------------
     * STEP 6
     * Display returned byte on PB0..PB7.
     * -------------------------------------------------------------
     */
    leds_write_byte(eeprom_read_value);


    /*
     * -------------------------------------------------------------
     * STEP 7
     * Green = PASS
     * Red   = FAIL
     * -------------------------------------------------------------
     */
    status_leds_show(
        eeprom_verify_ok
        ? STATUS_PASS
        : STATUS_FAIL
    );
}


/* ==========================================================================
 * RUN_TASK 4 (and 5) - given
 *
 *   reset / power-up : read-only path - shows the stored byte, writes nothing,
 *                      so a power-cycle proves the byte persisted
 *
 *   PA0              : one complete eeprom_write_verify_path()
 *
 *   eeprom_read_loop_enable = 1 (Live Expressions):
 *                      repeat a READ-ONLY transaction every 50 ms for a
 *                      steady scope trace.
 *
 *                      It never writes.
 * ========================================================================== */

#define EEPROM_READ_LOOP_MS  50u

volatile uint8_t eeprom_read_loop_enable = 0u;

static uint32_t rd_last = 0u;


void task4_setup(void)
{
    /*
     * Initialise:
     *
     * Task 1 GPIO / heartbeat
     * SPI peripheral
     * buttons / LEDs
     */
    task1_gpio_init();

    eeprom_spi_init();

    board_io_init();


    /*
     * IMPORTANT:
     *
     * On reset we READ ONLY.
     *
     * We deliberately do not automatically write here because otherwise
     * resetting the board would destroy the persistence demonstration.
     */
    eeprom_read_only_path();
}


void task4_loop(uint32_t now)
{
    /*
     * Keep Task 1 heartbeat/output running.
     */
    task1_gpio_update(now);


    /*
     * Read PA0 / PA3 button inputs.
     */
    read_inputs(now);


    /*
     * PA0:
     *
     * One press causes one EEPROM write/read/verify operation.
     */
    if (btn_start_edge)
    {
        btn_start_edge = 0u;

        eeprom_write_verify_path();
    }


    /*
     * PA3 is not used in Task 4.
     */
    btn_abort_edge = 0u;


    /*
     * Optional oscilloscope helper.
     *
     * Setting:
     *
     * eeprom_read_loop_enable = 1
     *
     * in Live Expressions repeatedly performs READ transactions.
     *
     * This gives a stable recurring waveform for the oscilloscope
     * without repeatedly writing to EEPROM.
     */
    if (eeprom_read_loop_enable &&
        ((uint32_t)(now - rd_last) >= EEPROM_READ_LOOP_MS))
    {
        rd_last = now;

        eeprom_read_only_path();
    }
}