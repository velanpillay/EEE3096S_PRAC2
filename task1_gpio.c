/**
  ******************************************************************************
  * @file    task1_gpio.c
  * @brief   TASK 1 : MEMORY-MAPPED GPIO ACCESS
  *
  * Drive PC13 as a digital output using EXPLICIT volatile register pointers,
  * built from addresses you find in RM0091:
  *
  *     register address = peripheral base address + register offset
  *
  * Do not use HAL_GPIO_Init(), HAL_GPIO_WritePin() or HAL_GPIO_TogglePin().
  * You must be able to show where every base and offset came from.
  ******************************************************************************
  */

#include "prac2a.h"

/* TODO 1.1
 * Peripheral base addresses from the STM32F0 memory map in RM0091.
 */
#define RCC_BASE_ADDR       0x40021000UL
#define GPIOC_BASE_ADDR     0x48000800UL


/* TODO 1.2
 * Register offsets from the RCC and GPIO register maps in RM0091.
 */
#define RCC_AHBENR_OFFSET   0x14UL
#define GPIO_MODER_OFFSET   0x00UL
#define GPIO_ODR_OFFSET     0x14UL
#define GPIO_BSRR_OFFSET    0x18UL
#define GPIO_BRR_OFFSET     0x28UL


/* --------------------------------------------------------------------------
 * TODO 1.3
 * Explicit volatile pointers to the hardware registers.
 * -------------------------------------------------------------------------- */

/* RCC AHB peripheral clock enable register */
static volatile uint32_t * const pRCC_AHBENR =
        (volatile uint32_t *)(RCC_BASE_ADDR + RCC_AHBENR_OFFSET);

/* GPIOC mode register */
static volatile uint32_t * const pGPIOC_MODER =
        (volatile uint32_t *)(GPIOC_BASE_ADDR + GPIO_MODER_OFFSET);

/* GPIOC output data register */
static volatile uint32_t * const pGPIOC_ODR =
        (volatile uint32_t *)(GPIOC_BASE_ADDR + GPIO_ODR_OFFSET);

/* GPIOC bit set/reset register */
static volatile uint32_t * const pGPIOC_BSRR =
        (volatile uint32_t *)(GPIOC_BASE_ADDR + GPIO_BSRR_OFFSET);

/* GPIOC bit reset register */
static volatile uint32_t * const pGPIOC_BRR =
        (volatile uint32_t *)(GPIOC_BASE_ADDR + GPIO_BRR_OFFSET);


/* --------------------------------------------------------------------------
 * TODO 1.4
 * Check our manually calculated addresses against the CMSIS definitions.
 * -------------------------------------------------------------------------- */

_Static_assert(RCC_BASE_ADDR   == RCC_BASE,
               "RCC base mismatch");

_Static_assert(GPIOC_BASE_ADDR == GPIOC_BASE,
               "GPIOC base mismatch");

_Static_assert(RCC_BASE_ADDR + RCC_AHBENR_OFFSET
               == (uint32_t)(uintptr_t)&RCC->AHBENR,
               "AHBENR offset");

_Static_assert(GPIOC_BASE_ADDR + GPIO_MODER_OFFSET
               == (uint32_t)(uintptr_t)&GPIOC->MODER,
               "MODER offset");

_Static_assert(GPIOC_BASE_ADDR + GPIO_ODR_OFFSET
               == (uint32_t)(uintptr_t)&GPIOC->ODR,
               "ODR offset");

_Static_assert(GPIOC_BASE_ADDR + GPIO_BSRR_OFFSET
               == (uint32_t)(uintptr_t)&GPIOC->BSRR,
               "BSRR offset");

_Static_assert(GPIOC_BASE_ADDR + GPIO_BRR_OFFSET
               == (uint32_t)(uintptr_t)&GPIOC->BRR,
               "BRR offset");


volatile uint32_t task1_half_period_ms = 5u;
volatile uint32_t task1_toggle_count   = 0u;

static uint32_t t1_last = 0u;


/* ==========================================================================
 * GPIO INITIALISATION
 * ========================================================================== */

void task1_gpio_init(void)
{
    uint32_t moder;

    /* ----------------------------------------------------------------------
     * TODO 1.5
     * Enable the GPIOC peripheral clock.
     *
     * RCC_AHBENR bit 19 = GPIOCEN
     *
     * Read-modify-write:
     *      existing register value OR bit 19
     *
     * This preserves all other enabled peripheral clocks.
     * ---------------------------------------------------------------------- */

    *pRCC_AHBENR |= (1UL << 19);

    /*
     * Read the register back.
     *
     * This ensures that the peripheral clock write has completed before
     * accessing GPIOC.
     */
    (void)*pRCC_AHBENR;


    /* ----------------------------------------------------------------------
     * TODO 1.6
     * Configure PC13 as a general-purpose output.
     *
     * Each GPIO pin has TWO mode bits.
     *
     * PC13 therefore uses:
     *
     *      MODER[27:26]
     *
     * because:
     *
     *      13 * 2 = 26
     *
     * GPIO mode:
     *
     *      00 = input
     *      01 = general-purpose output
     *      10 = alternate function
     *      11 = analog
     *
     * We want 01.
     * ---------------------------------------------------------------------- */

    moder = *pGPIOC_MODER;

    /* Clear both PC13 mode bits */
    moder &= ~(3UL << (13u * 2u));

    /* Set mode to 01 = general-purpose output */
    moder |=  (1UL << (13u * 2u));

    *pGPIOC_MODER = moder;


    /* ----------------------------------------------------------------------
     * TODO 1.7
     * Start PC13 LOW.
     *
     * GPIOC_BRR resets an output bit when a 1 is written to that position.
     * ---------------------------------------------------------------------- */

    *pGPIOC_BRR = (1UL << 13);
}


/* ==========================================================================
 * GPIO UPDATE
 * ========================================================================== */

void task1_gpio_update(uint32_t now)
{
    /*
     * Wait until one half-period has elapsed.
     *
     * task1_half_period_ms = 5 ms
     *
     * Therefore:
     *
     * HIGH for 5 ms
     * LOW  for 5 ms
     *
     * Full period = 10 ms
     * Frequency   = 100 Hz
     */
    if ((uint32_t)(now - t1_last) < task1_half_period_ms)
    {
        return;
    }

    t1_last = now;


    /* ----------------------------------------------------------------------
     * TODO 1.8
     * Read PC13's current output state from ODR.
     *
     * If PC13 is HIGH:
     *      write bit 13 to BRR -> make it LOW.
     *
     * If PC13 is LOW:
     *      write bit 13 to BSRR -> make it HIGH.
     *
     * BSRR/BRR are useful because they change only the requested GPIO bit.
     * ---------------------------------------------------------------------- */

    if ((*pGPIOC_ODR & (1UL << 13)) != 0u)
    {
        /* PC13 currently HIGH -> drive it LOW */
        *pGPIOC_BRR = (1UL << 13);
    }
    else
    {
        /* PC13 currently LOW -> drive it HIGH */
        *pGPIOC_BSRR = (1UL << 13);
    }


    task1_toggle_count++;
}


/* ==========================================================================
 * RUN_TASK 1 - given
 * ========================================================================== */

void task1_setup(void)
{
    task1_gpio_init();
}


void task1_loop(uint32_t now)
{
    task1_gpio_update(now);
}
