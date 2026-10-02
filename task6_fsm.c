/**
  ******************************************************************************
  * @file    task6_fsm.c
  * @brief   TASK 6 : NON-BLOCKING EEPROM TRANSACTION STATE MACHINE
  *
  * Restructure the Task 4 transaction so the main loop never stops:
  *
  *   request -> write-enable -> write -> wait for EEPROM
  *           -> read-back -> verify -> result
  *
  * Rules (handout, Task 6):
  *   - No HAL_Delay(), and no software busy-wait for the EEPROM's internal
  *     write cycle. You may use HAL_GetTick() to decide when the next status
  *     check is due.
  *   - Each call does a small amount of work, updates the state, and returns.
  *   - Do not put the whole transaction inside one blocking function called
  *     from the loop.
  *   - While a transaction is in progress the loop must still respond to PA3.
  *
  * Controls:
  *   PA0       = start transaction
  *   PA3       = abort transaction
  *   PB0..PB7  = last byte read
  *   PB11      = green success LED
  *   PB10      = red failure LED
  ******************************************************************************
  */

#include "prac2a.h"


/* ==========================================================================
 * State machine states
 * ========================================================================== */

typedef enum
{
    EE_IDLE = 0,

    EE_PREPARE,

    EE_WRITE_ENABLE,

    EE_WRITE_TRANSACTION,

    EE_WAIT_READY,

    EE_READ_TRANSACTION,

    EE_VERIFY,

    EE_SUCCESS,

    EE_FAILURE

} ee_state_t;


/* ==========================================================================
 * Debug / state variables
 * ========================================================================== */

volatile uint8_t ee_state       = EE_IDLE;
volatile uint8_t ee_last_read   = 0u;
volatile uint8_t ee_use_fsm     = 1u;


/*
 * Remember which result should be displayed on the status LEDs.
 *
 * STATUS_OFF  = no result / transaction running / aborted
 * STATUS_PASS = verification successful
 * STATUS_FAIL = verification failed
 */
static status_led_t ee_result_status = STATUS_OFF;


/*
 * Timing variables.
 *
 * ee_write_start_ms:
 *      when the EEPROM write transaction was started.
 *
 * ee_last_status_poll_ms:
 *      when we last checked the EEPROM status register.
 */
static uint32_t ee_write_start_ms      = 0u;
static uint32_t ee_last_status_poll_ms = 0u;


/*
 * Poll EEPROM approximately once every millisecond while it is busy.
 *
 * The EEPROM status bit still determines whether the write has actually
 * completed. Time is only used to schedule status checks.
 */
#define EEPROM_STATUS_POLL_MS  1u


/* ==========================================================================
 * State machine
 * ========================================================================== */

void update_eeprom_state_machine(uint32_t now)
{
    uint8_t status;
    int32_t i;


    /* ======================================================================
     * ABORT
     *
     * PA3 has priority over everything else.
     *
     * If the user presses PA3 while a transaction is active, abandon the
     * SOFTWARE transaction and return to IDLE.
     *
     * Important:
     * If the EEPROM's internal write has already started, software cannot
     * undo that physical EEPROM write. We simply stop waiting for it.
     * ====================================================================== */

    if (btn_abort_edge)
    {
        btn_abort_edge = 0u;

        /*
         * Leave SPI chip select in its safe inactive state.
         */
        eeprom_cs_high();

        /*
         * Return controller to idle.
         */
        ee_state = EE_IDLE;

        /*
         * Clear current result indication after abort.
         */
        ee_result_status = STATUS_OFF;

        return;
    }


    /* ======================================================================
     * STATE MACHINE
     * ====================================================================== */

    switch ((ee_state_t)ee_state)
    {

        /* ==================================================================
         * IDLE
         *
         * Wait here until PA0 is pressed.
         * ================================================================== */

        case EE_IDLE:

            if (btn_start_edge)
            {
                /*
                 * Consume PA0 press.
                 */
                btn_start_edge = 0u;

                /*
                 * Clear previous result before starting new transaction.
                 */
                ee_result_status = STATUS_OFF;

                /*
                 * Clear verification result.
                 */
                eeprom_verify_ok = 0u;

                /*
                 * Start transaction.
                 */
                ee_state = EE_PREPARE;
            }

            break;


        /* ==================================================================
         * PREPARE
         *
         * Check that the EEPROM is ready before beginning a new write.
         *
         * This is useful if the previous software operation was aborted while
         * the EEPROM was still completing an internal write.
         * ================================================================== */

        case EE_PREPARE:

            status = eeprom_read_status();

            /*
             * RDY = 0 means EEPROM is ready.
             */
            if ((status & EEPROM_SR_RDY) == 0u)
            {
                ee_state = EE_WRITE_ENABLE;
            }

            /*
             * Otherwise remain in PREPARE.
             *
             * There is NO loop here.
             * Control immediately returns to main().
             */

            break;


        /* ==================================================================
         * WRITE ENABLE
         *
         * Set the EEPROM Write Enable Latch.
         * ================================================================== */

        case EE_WRITE_ENABLE:

            /*
             * WREN is a short SPI transaction.
             */
            eeprom_write_enable();


            /*
             * Read status once to verify WEL was actually set.
             */
            status = eeprom_read_status();


            /*
             * WEL = 1 means EEPROM accepted write-enable command.
             */
            if ((status & EEPROM_SR_WEL) != 0u)
            {
                ee_state = EE_WRITE_TRANSACTION;
            }
            else
            {
                /*
                 * Write enable failed.
                 */
                ee_state = EE_FAILURE;
            }

            break;


        /* ==================================================================
         * WRITE TRANSACTION
         *
         * IMPORTANT:
         *
         * We DO NOT call:
         *
         *      eeprom_write_byte(...)
         *
         * because the Task 4 version blocks while waiting for the EEPROM.
         *
         * Instead, we send only the WRITE transaction here and then change
         * state to EE_WAIT_READY.
         * ================================================================== */

        case EE_WRITE_TRANSACTION:

            /*
             * Check address is valid.
             */
            if (eeprom_test_addr >= EEPROM_SIZE_BYTES)
            {
                ee_state = EE_FAILURE;
                break;
            }


            /*
             * Select EEPROM.
             */
            eeprom_cs_low();


            /*
             * Send WRITE command.
             */
            spi_transfer(EEPROM_CMD_WRITE);


            /*
             * Send EEPROM address MSB first.
             *
             * For EEPROM_ADDR_BYTES = 2:
             *
             * first byte  = address >> 8
             * second byte = address >> 0
             */
            for (i = ((int32_t)EEPROM_ADDR_BYTES - 1);
                 i >= 0;
                 i--)
            {
                spi_transfer(
                    (uint8_t)(
                        (eeprom_test_addr >>
                        (8u * (uint32_t)i))
                        & 0xFFu
                    )
                );
            }


            /*
             * Send the byte that must be written.
             */
            spi_transfer(eeprom_test_byte);


            /*
             * End WRITE transaction.
             *
             * The rising edge of CS causes the EEPROM to begin its internal
             * programming cycle.
             */
            eeprom_cs_high();


            /*
             * Record when internal EEPROM writing started.
             */
            ee_write_start_ms = now;


            /*
             * Start status polling timing from now.
             */
            ee_last_status_poll_ms = now;


            /*
             * Move immediately to non-blocking wait state.
             */
            ee_state = EE_WAIT_READY;

            break;


        /* ==================================================================
         * WAIT FOR EEPROM
         *
         * This is the important NON-BLOCKING state.
         *
         * There is:
         *
         *      NO while loop
         *      NO HAL_Delay()
         *
         * We periodically check the status register and then return control
         * to the main loop.
         * ================================================================== */

        case EE_WAIT_READY:

            /*
             * First protect against a missing / non-responsive EEPROM.
             *
             * This timeout does NOT mean the write completed.
             *
             * Successful completion is ONLY determined using the EEPROM
             * RDY status bit.
             */
            if ((uint32_t)(now - ee_write_start_ms)
                >= EEPROM_WRITE_TIMEOUT_MS)
            {
                eeprom_timeout_count++;

                ee_state = EE_FAILURE;

                break;
            }


            /*
             * Do not poll continuously.
             *
             * If it is not yet time for another status check, simply leave
             * this state-machine update and return to main().
             */
            if ((uint32_t)(now - ee_last_status_poll_ms)
                < EEPROM_STATUS_POLL_MS)
            {
                break;
            }


            /*
             * Record this status check time.
             */
            ee_last_status_poll_ms = now;


            /*
             * Read EEPROM status ONCE.
             */
            status = eeprom_read_status();


            /*
             * RDY bit:
             *
             * 1 = EEPROM still busy
             * 0 = EEPROM write finished
             */
            if ((status & EEPROM_SR_RDY) == 0u)
            {
                /*
                 * Record actual write waiting time.
                 */
                eeprom_write_wait_ms =
                    (uint32_t)(now - ee_write_start_ms);


                /*
                 * EEPROM says programming is complete.
                 */
                ee_state = EE_READ_TRANSACTION;
            }

            /*
             * If RDY is still 1, remain in EE_WAIT_READY.
             *
             * No loop occurs here.
             */

            break;


        /* ==================================================================
         * READ TRANSACTION
         *
         * Read back the byte that was just written.
         * ================================================================== */

        case EE_READ_TRANSACTION:

            /*
             * eeprom_read_byte() performs only one short SPI transaction.
             *
             * It does NOT contain the blocking EEPROM-write wait from Task 4.
             */
            ee_last_read =
                eeprom_read_byte(eeprom_test_addr);


            /*
             * Keep Task 4 debugging variable updated too.
             */
            eeprom_read_value = ee_last_read;


            /*
             * Move to verification.
             */
            ee_state = EE_VERIFY;

            break;


        /* ==================================================================
         * VERIFY
         *
         * Compare returned EEPROM byte with expected byte.
         * ================================================================== */

        case EE_VERIFY:

            if (ee_last_read == eeprom_test_byte)
            {
                /*
                 * Verification passed.
                 */
                eeprom_verify_ok = 1u;

                ee_state = EE_SUCCESS;
            }
            else
            {
                /*
                 * Verification failed.
                 */
                eeprom_verify_ok = 0u;

                ee_state = EE_FAILURE;
            }

            break;


        /* ==================================================================
         * SUCCESS
         *
         * Successful transaction.
         * ================================================================== */

        case EE_SUCCESS:

            /*
             * Latch green result.
             */
            ee_result_status = STATUS_PASS;


            /*
             * Return controller to idle.
             *
             * The green result remains displayed because ee_result_status
             * stores the result independently of ee_state.
             */
            ee_state = EE_IDLE;

            break;


        /* ==================================================================
         * FAILURE
         *
         * Transaction failed.
         * ================================================================== */

        case EE_FAILURE:

            /*
             * Latch red result.
             */
            ee_result_status = STATUS_FAIL;


            /*
             * Make sure EEPROM is deselected.
             */
            eeprom_cs_high();


            /*
             * Return to idle so PA0 may start another transaction.
             *
             * Red remains displayed through ee_result_status.
             */
            ee_state = EE_IDLE;

            break;


        /* ==================================================================
         * SAFETY / INVALID STATE
         * ================================================================== */

        default:

            /*
             * Put SPI bus into safe state.
             */
            eeprom_cs_high();


            /*
             * Clear result.
             */
            ee_result_status = STATUS_OFF;


            /*
             * Recover by returning to IDLE.
             */
            ee_state = EE_IDLE;

            break;
    }
}


/* ==========================================================================
 * Outputs
 * ========================================================================== */

void update_outputs(void)
{
    /*
     * PB0..PB7 always display the last byte successfully read from EEPROM.
     */
    leds_write_byte(ee_last_read);


    /*
     * PB11 green = successful verification.
     *
     * PB10 red   = failed verification.
     *
     * During a transaction or after an abort:
     * both are off.
     */
    status_leds_show(ee_result_status);
}


/* ==========================================================================
 * RUN_TASK 6 - given
 *
 * The main loop has exactly the shape the handout asks for:
 *
 *     read_inputs();
 *     update_eeprom_state_machine();
 *     update_outputs();
 *
 * Set ee_use_fsm = 0 in Live Expressions to run the Task 4 blocking path on
 * PA0 instead - useful when you explain why yours is non-blocking.
 *
 * PC13 keeps toggling as a heartbeat; watch it on the scope in both modes.
 * ========================================================================== */

void task6_setup(void)
{
    /*
     * Initialise GPIO heartbeat.
     */
    task1_gpio_init();


    /*
     * Initialise hardware SPI used by EEPROM.
     */
    eeprom_spi_init();


    /*
     * Initialise PA0 / PA3 and status LEDs.
     */
    board_io_init();


    /*
     * IMPORTANT:
     *
     * On boot, READ ONLY.
     *
     * This preserves the EEPROM persistence test because resetting the board
     * does not automatically overwrite the stored byte.
     */
    eeprom_read_only_path();


    /*
     * Save boot-time EEPROM value as last byte read.
     */
    ee_last_read = eeprom_read_value;


    /*
     * State machine starts idle.
     */
    ee_state = EE_IDLE;


    /*
     * Preserve the boot-time persistence result.
     *
     * If the stored EEPROM value equals the expected test byte:
     *      green
     *
     * otherwise:
     *      red
     */
    if (eeprom_verify_ok)
    {
        ee_result_status = STATUS_PASS;
    }
    else
    {
        ee_result_status = STATUS_FAIL;
    }


    /*
     * Ensure outputs immediately display boot result.
     */
    update_outputs();
}


/* ==========================================================================
 * Main Task 6 loop
 * ========================================================================== */

void task6_loop(uint32_t now)
{
    /*
     * Keep PC13 heartbeat running.
     *
     * This is useful for demonstrating that the main loop remains responsive
     * while the EEPROM performs its internal write operation.
     */
    task1_gpio_update(now);


    /*
     * Read PA0 / PA3.
     */
    read_inputs(now);


    /*
     * Normal Task 6 FSM.
     */
    if (ee_use_fsm)
    {
        update_eeprom_state_machine(now);

        update_outputs();
    }

    /*
     * Debug comparison:
     *
     * ee_use_fsm = 0 runs the old Task 4 blocking implementation.
     */
    else
    {
        if (btn_start_edge)
        {
            btn_start_edge = 0u;


            /*
             * Task 4 implementation:
             *
             * this blocks while the EEPROM is internally writing.
             */
            eeprom_write_verify_path();


            /*
             * Save result for PB0..PB7.
             */
            ee_last_read = eeprom_read_value;
        }


        /*
         * PA3 abort functionality belongs to Task 6 FSM mode only.
         */
        btn_abort_edge = 0u;
    }
}