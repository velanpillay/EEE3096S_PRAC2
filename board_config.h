/**
  ******************************************************************************
  * @file    board_config.h
  * @brief   Board facts, and the values you must look up.
  *
  * TODO 0 - START HERE.
  *
  * Values marked TODO have to be FOUND in the document named next to them.
  * The handout is explicit: "Where a task asks for a register field,
  * peripheral address, alternate function, command value, timing requirement,
  * or status bit, do not guess it."
  *
  * The project compiles with the placeholders, but the board will not work
  * until they are correct. That is deliberate.
  *
  * Two things below are GIVEN rather than left as TODOs: the SPI pin mapping
  * and the EEPROM address format. The handout is wrong about both on this
  * board, so you could not find the right answer in your documents. README
  * section 5 explains how to confirm them on the bench - do that.
  ******************************************************************************
  */

#ifndef __BOARD_CONFIG_H
#define __BOARD_CONFIG_H

#include "stm32f0xx.h"

/* ==========================================================================
 * 1. CLOCK
 * ========================================================================== */

/* TODO 2.1  The kernel clock frequency of the SPI peripheral in THIS build.
 *           Read SystemClock_Config() in main.c, then follow the clock tree in
 *           the RCC chapter of RM0091 to the bus the SPI peripheral sits on.
 *           Do not assume it.
 *
 * PARTNER / TASK 2
 */
#define PCLK1_HZ                0UL         /* <- TODO */


/* ==========================================================================
 * 2. SPI PINS - GIVEN (see README section 5, and verify by continuity)
 * ========================================================================== */

#define EE_SPI_GPIO             GPIOB

#define EE_PIN_CS               12u         /* EEPROM pin 1 (CS#) */
#define EE_PIN_SCK              13u         /* EEPROM pin 6 (SCK) */
#define EE_PIN_MISO             14u         /* EEPROM pin 2 (SO)  */
#define EE_PIN_MOSI             15u         /* EEPROM pin 5 (SI)  */

#define EE_CS_MASK              (1UL << EE_PIN_CS)


/* TODO 2.2  Which SPI peripheral do these pins belong to, and which
 *           alternate-function NUMBER connects it to them?
 *
 * PARTNER / TASK 2
 */
#define EE_SPI                  ((SPI_TypeDef *)0x00000000UL) /* <- TODO */
#define EE_SPI_AF               0xFFu                         /* <- TODO */


/* ==========================================================================
 * 3. SPI BAUD RATE
 * ========================================================================== */

/* TODO 2.3  Choose BR[2:0] for an SCK of approximately 250 kHz from
 *           PCLK1_HZ.
 *
 * PARTNER / TASK 2
 */
#define EE_SPI_BR               0UL         /* <- TODO */

#define EE_SCK_HZ_PREDICTED     (PCLK1_HZ >> (EE_SPI_BR + 1U))


/* ==========================================================================
 * 4. EEPROM
 *
 * YOUR SECTION - TASKS 4 TO 6
 * ========================================================================== */

/*
 * GIVEN:
 *
 * EEPROM uses TWO address bytes.
 *
 * Address is transmitted:
 *
 *      MSB first
 *      then LSB
 *
 * Total EEPROM size = 8192 bytes.
 */
#define EEPROM_ADDR_BYTES       2u
#define EEPROM_SIZE_BYTES       8192u


/* --------------------------------------------------------------------------
 * TODO 4.1 - EEPROM instruction opcodes
 *
 * From EEPROM datasheet instruction table.
 * -------------------------------------------------------------------------- */

/*
 * Write Enable
 */
#define EEPROM_CMD_WREN         0x06u


/*
 * Write Disable
 */
#define EEPROM_CMD_WRDI         0x04u


/*
 * Read Status Register
 */
#define EEPROM_CMD_RDSR         0x05u


/*
 * Write Status Register
 */
#define EEPROM_CMD_WRSR         0x01u


/*
 * Read EEPROM memory
 */
#define EEPROM_CMD_READ         0x03u


/*
 * Write EEPROM memory
 */
#define EEPROM_CMD_WRITE        0x02u


/* --------------------------------------------------------------------------
 * TODO 4.2 - EEPROM Status Register
 *
 * Status register:
 *
 * Bit 0 = RDY
 * Bit 1 = WEL
 *
 * RDY:
 *
 *      1 = EEPROM busy with internal write
 *      0 = EEPROM ready
 *
 * WEL:
 *
 *      1 = Write Enable Latch set
 *      0 = writes disabled
 * -------------------------------------------------------------------------- */

/*
 * RDY = bit 0
 *
 * Binary:
 *
 *      0000 0001
 *
 * Hex:
 *
 *      0x01
 */
#define EEPROM_SR_RDY           0x01u


/*
 * WEL = bit 1
 *
 * Binary:
 *
 *      0000 0010
 *
 * Hex:
 *
 *      0x02
 */
#define EEPROM_SR_WEL           0x02u


/*
 * Maximum amount of time Task 4 / Task 6 will tolerate the EEPROM
 * remaining busy.
 *
 * IMPORTANT:
 *
 * This is ONLY a timeout / hang guard.
 *
 * Successful write completion must still be determined using:
 *
 *      EEPROM_SR_RDY
 *
 * not simply elapsed time.
 */
#define EEPROM_WRITE_TIMEOUT_MS 50u


/* ==========================================================================
 * 5. BOARD I/O - GIVEN (UCT board: Board.md)
 * ========================================================================== */

#define LED_BYTE_GPIO           GPIOB
#define LED_BYTE_MASK           0x00FFu     /* PB0..PB7 */

#define LED_RED_PIN             10u         /* PB10 */
#define LED_GREEN_PIN           11u         /* PB11 */

#define BTN_GPIO                GPIOA

#define BTN_START_PIN           0u          /* PA0 / SW0, active low */
#define BTN_ABORT_PIN           3u          /* PA3 / SW3, active low */

#define SCOPE_GPIO              GPIOC
#define SCOPE_PIN               13u         /* PC13, header P1 */


/* ==========================================================================
 * 6. GROUP VALUES
 * ========================================================================== */

/* TODO 3.1
 *
 * PARTNER / SHARED TASK 3
 *
 * The last three decimal digits of each student number.
 *
 * Do NOT use leading zeros.
 */
#define STUDENT_N1              0u          /* <- TODO */
#define STUDENT_N2              0u          /* <- TODO */


/*
 * Group test byte B.
 */
#define TEST_BYTE_B_RAW         ((((STUDENT_N1 ^ STUDENT_N2) + 0x3Du)) % 256u)

#define TEST_BYTE_B             (((TEST_BYTE_B_RAW == 0x00u) ||   \
                                  (TEST_BYTE_B_RAW == 0xFFu))      \
                                 ? (TEST_BYTE_B_RAW ^ 0x5Au)       \
                                 : TEST_BYTE_B_RAW)


/*
 * Group EEPROM address A.
 */
#define EEPROM_ADDR_A           ((STUDENT_N1 + 3u * STUDENT_N2) % 256u)


/* ==========================================================================
 * Helpers
 * ========================================================================== */

/*
 * Fields that use two bits per GPIO pin:
 *
 * MODER
 * OSPEEDR
 * PUPDR
 */
#define MODER2(pin, val)        ((uint32_t)(val) << ((pin) * 2u))

#define MODER2_MASK(pin)        (3UL << ((pin) * 2u))


/*
 * Alternate function fields for pins 8..15.
 */
#define AFRH4(pin, af)          ((uint32_t)(af) << (((pin) - 8u) * 4u))

#define AFRH4_MASK(pin)         (0xFUL << (((pin) - 8u) * 4u))


#endif /* __BOARD_CONFIG_H */