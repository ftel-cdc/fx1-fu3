/**
 * @file neon_syscsr.c
 * @brief Implementation for SYSCSR.
 * @details
 * *NEON SYSCSR backend for GT32U2086.
 *
 * The register, reset state, physical pin and mux pin data are taken from GT32U2086 LL. HAL API uses the
 * function logic seen for leaving the upper layer MUX_0..3.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "syscsr/neon_syscsr.h"
#include "syscsr/platform/gt32u2086/neon_syscsr_internal.h"
#include <string.h>

#include <stddef.h>

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define SYSCSR_MUX_VALUE_MASK 0x3UL
#define SYSCSR_PDRV_VALUE_MASK 0x3UL
#define SYSCSR_GPIO_PIN_MASK 0xFFUL
#define SYSCSR_PAD_LEGACY_COUNT 12UL
#define SYSCSR_EXT_PAD_MASK 0x00FFFFFFUL
#define SYSCSR_EXT_IO_MASK 0x0000FFFFUL
#define SYSCSR_RESET_ATTEMPTS 64UL
#define SYSCSR_RESET_POLLS 20000UL
#define SYSCSR_FLASH_INIT_CMD                                                                                          \
    (((1UL << FLASH_COMMAND_CMD_OPCODE_Pos) & FLASH_COMMAND_CMD_OPCODE_Msk) | FLASH_COMMAND_START_Msk)

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef struct {
    volatile uint32_t *io_config;
    volatile uint32_t *pad_control0;
    volatile uint32_t *pad_control1;
    volatile uint32_t *pad_control2;
    volatile uint32_t *pad_control3;
    volatile uint32_t *pad_control4;
    volatile uint32_t *pad_control5;
    volatile uint32_t *pad_control6;
    volatile uint32_t *pad_control7;
    volatile uint32_t *io_config_extended;
} gt32u2086_syscsr_register_set_t;

typedef struct {
    uint32_t pad_control0;
    uint32_t pad_control1;
    uint32_t pad_control2;
    uint32_t pad_control3;
    uint32_t pad_control4;
} gt32u2086_syscsr_pad_reset_t;

/*==================================================================================================
*                                         LOCAL VARIABLES
==================================================================================================*/
/* Reset values implemented by the GT32U2086 v8 SYSCSR model. */
static const gt32u2086_syscsr_pad_reset_t gt32u2086_syscsr_pad_reset[] = {
    [SYSCSR_REGISTER_GROUP_LEFT] = {0x00F86C3FUL, 0x00FFFFFFUL, 0UL, 0UL, 0UL},
    [SYSCSR_REGISTER_GROUP_BOTTOM] = {0x00C5007FUL, 0x00FFFFFFUL, 0UL, 0UL, 0UL},
    [SYSCSR_REGISTER_GROUP_RIGHT] = {0x00F80580UL, 0x00FFFFFFUL, 0UL, 0UL, 0UL},
    [SYSCSR_REGISTER_GROUP_TOP] = {0x00E16E0FUL, 0x00FFFFFFUL, 0UL, 0UL, 0UL},
};

#define SYSCSR_PAD_MAP(pin, group, index) {SYSCSR_PIN_##pin, SYSCSR_REGISTER_GROUP_##group, index}

/* Package pin → side/index of DIE pin. Each side has 24 selector; this table copied from iopad_map.h
 * of VP, not deduced from the package pin number. */
static const gt32u2086_syscsr_physical_pad_map_t gt32u2086_syscsr_physical_pad_map[] = {
    SYSCSR_PAD_MAP(6, LEFT, 5),    SYSCSR_PAD_MAP(11, LEFT, 11),  SYSCSR_PAD_MAP(12, LEFT, 12),
    SYSCSR_PAD_MAP(13, LEFT, 13),  SYSCSR_PAD_MAP(14, LEFT, 14),  SYSCSR_PAD_MAP(18, LEFT, 19),
    SYSCSR_PAD_MAP(19, LEFT, 20),  SYSCSR_PAD_MAP(20, LEFT, 21),  SYSCSR_PAD_MAP(21, BOTTOM, 0),
    SYSCSR_PAD_MAP(22, BOTTOM, 1), SYSCSR_PAD_MAP(23, BOTTOM, 2), SYSCSR_PAD_MAP(24, BOTTOM, 3),
    SYSCSR_PAD_MAP(25, BOTTOM, 4), SYSCSR_PAD_MAP(26, BOTTOM, 5), SYSCSR_PAD_MAP(27, BOTTOM, 6),
    SYSCSR_PAD_MAP(48, RIGHT, 7),  SYSCSR_PAD_MAP(49, RIGHT, 8),  SYSCSR_PAD_MAP(50, RIGHT, 9),
    SYSCSR_PAD_MAP(51, RIGHT, 10), SYSCSR_PAD_MAP(58, RIGHT, 19), SYSCSR_PAD_MAP(59, RIGHT, 20),
    SYSCSR_PAD_MAP(60, RIGHT, 21), SYSCSR_PAD_MAP(61, TOP, 0),    SYSCSR_PAD_MAP(62, TOP, 1),
    SYSCSR_PAD_MAP(63, TOP, 2),    SYSCSR_PAD_MAP(64, TOP, 3),    SYSCSR_PAD_MAP(68, TOP, 8),
    SYSCSR_PAD_MAP(69, TOP, 9),    SYSCSR_PAD_MAP(70, TOP, 10),   SYSCSR_PAD_MAP(71, TOP, 11),
    SYSCSR_PAD_MAP(72, TOP, 12),   SYSCSR_PAD_MAP(73, TOP, 13),   SYSCSR_PAD_MAP(74, TOP, 14),
    SYSCSR_PAD_MAP(75, TOP, 15),   SYSCSR_PAD_MAP(76, TOP, 16),   SYSCSR_PAD_MAP(80, TOP, 21),
};

#undef SYSCSR_PAD_MAP

/* mux table by kGT32U2086PadMap[] in new VP. */
#define SYSCSR_MUX_MAP(pin, mux0, mux1, mux2, mux3)                                                                    \
    {                                                                                                                  \
        SYSCSR_PIN_##pin, {                                                                                            \
            SYSCSR_PIN_FUNCTION_##mux0, SYSCSR_PIN_FUNCTION_##mux1, SYSCSR_PIN_FUNCTION_##mux2,                        \
                SYSCSR_PIN_FUNCTION_##mux3                                                                             \
        }                                                                                                              \
    }

static const gt32u2086_syscsr_pin_function_map_t gt32u2086_syscsr_function_map[] = {
    SYSCSR_MUX_MAP(6, JTAG_TDI, GPIO, PWM_T4, NONE),
    SYSCSR_MUX_MAP(11, JTAG_TSM, GPIO, PWM_T5, NONE),
    SYSCSR_MUX_MAP(12, JTAG_TDO, GPIO, UART2_TX, NONE),
    SYSCSR_MUX_MAP(13, JTAG_TRSTN, GPIO, UART2_RX, NONE),
    SYSCSR_MUX_MAP(14, JTAG_TCLK, GPIO, NONE, NONE),
    SYSCSR_MUX_MAP(18, GPIO, I2C0_SDA, PWM_T6, NONE),
    SYSCSR_MUX_MAP(19, GPIO, I2C0_SCL, PWM_T7, NONE),
    SYSCSR_MUX_MAP(20, GPIO, SPI1_SCK, NONE, NONE),
    SYSCSR_MUX_MAP(21, GPIO, SPI1_MISO, NONE, NONE),
    SYSCSR_MUX_MAP(22, GPIO, SPI1_MOSI, NONE, NONE),
    SYSCSR_MUX_MAP(23, GPIO, SPI1_SSN, NONE, NONE),
    SYSCSR_MUX_MAP(24, GPIO, UART3_TX, NONE, NONE),
    SYSCSR_MUX_MAP(25, GPIO, UART3_RX, NONE, NONE),
    SYSCSR_MUX_MAP(26, GPIO, UART4_TX, NONE, NONE),
    SYSCSR_MUX_MAP(27, GPIO, NONE, NONE, NONE),
    SYSCSR_MUX_MAP(48, GPIO, UART4_RX, PWM_T0, NONE),
    SYSCSR_MUX_MAP(49, GPIO, PWM_T1, NONE, NONE),
    SYSCSR_MUX_MAP(50, UART0_TX, GPIO, NONE, NONE),
    SYSCSR_MUX_MAP(51, UART0_RX, GPIO, NONE, NONE),
    SYSCSR_MUX_MAP(58, GPIO, UART1_TX, NONE, NONE),
    SYSCSR_MUX_MAP(59, GPIO, UART1_RX, NONE, NONE),
    SYSCSR_MUX_MAP(60, GPIO, SPI0_SCK, NONE, NONE),
    SYSCSR_MUX_MAP(61, GPIO, SPI0_MISO, NONE, NONE),
    SYSCSR_MUX_MAP(62, GPIO, SPI0_MOSI, NONE, NONE),
    SYSCSR_MUX_MAP(63, GPIO, SPI0_SSN, NONE, NONE),
    SYSCSR_MUX_MAP(64, QSPI_D3, GPIO, NONE, NONE),
    SYSCSR_MUX_MAP(68, QSPI_SCK, GPIO, NONE, NONE),
    SYSCSR_MUX_MAP(69, QSPI_D2, GPIO, NONE, NONE),
    SYSCSR_MUX_MAP(70, QSPI_D1, GPIO, NONE, NONE),
    SYSCSR_MUX_MAP(71, QSPI_D0, GPIO, NONE, NONE),
    SYSCSR_MUX_MAP(72, QSPI_SSN, GPIO, NONE, NONE),
    SYSCSR_MUX_MAP(73, GPIO, PWM0, NONE, NONE),
    SYSCSR_MUX_MAP(74, GPIO, PWM1, NONE, NONE),
    SYSCSR_MUX_MAP(75, PLL_CLOCK_OUT, GPIO, NONE, NONE),
    SYSCSR_MUX_MAP(76, GPIO, OSC32_CLOCK_FAULT, NONE, NONE),
    SYSCSR_MUX_MAP(80, GPIO, NONE, NONE, NONE),
};

#undef SYSCSR_MUX_MAP

#define SYSCSR_GPIO_MAP(port, bit, pin) {GPIO##port, bit, SYSCSR_PIN_##pin}

/* GPIO port/bit → battery pack according to iopad_map.h of new VP. */
static const gt32u2086_syscsr_gpio_map_t gt32u2086_syscsr_gpio_map[] = {
    SYSCSR_GPIO_MAP(A, 0, PA0), SYSCSR_GPIO_MAP(A, 1, PA1), SYSCSR_GPIO_MAP(A, 2, PA2), SYSCSR_GPIO_MAP(A, 3, PA3),
    SYSCSR_GPIO_MAP(A, 4, PA4), SYSCSR_GPIO_MAP(A, 5, PA5), SYSCSR_GPIO_MAP(A, 6, PA6), SYSCSR_GPIO_MAP(A, 7, PA7),
    SYSCSR_GPIO_MAP(B, 0, PB0), SYSCSR_GPIO_MAP(B, 1, PB1), SYSCSR_GPIO_MAP(B, 2, PB2), SYSCSR_GPIO_MAP(B, 3, PB3),
    SYSCSR_GPIO_MAP(B, 4, PB4), SYSCSR_GPIO_MAP(B, 5, PB5), SYSCSR_GPIO_MAP(B, 6, PB6), SYSCSR_GPIO_MAP(B, 7, PB7),
    SYSCSR_GPIO_MAP(C, 0, PC0), SYSCSR_GPIO_MAP(C, 1, PC1), SYSCSR_GPIO_MAP(C, 2, PC2), SYSCSR_GPIO_MAP(C, 3, PC3),
    SYSCSR_GPIO_MAP(C, 4, PC4), SYSCSR_GPIO_MAP(C, 5, PC5), SYSCSR_GPIO_MAP(C, 6, PC6), SYSCSR_GPIO_MAP(C, 7, PC7),
    SYSCSR_GPIO_MAP(D, 0, PD0), SYSCSR_GPIO_MAP(D, 1, PD1), SYSCSR_GPIO_MAP(D, 2, PD2), SYSCSR_GPIO_MAP(D, 3, PD3),
    SYSCSR_GPIO_MAP(D, 4, PD4), SYSCSR_GPIO_MAP(D, 5, PD5), SYSCSR_GPIO_MAP(D, 6, PD6), SYSCSR_GPIO_MAP(D, 7, PD7),
    SYSCSR_GPIO_MAP(E, 0, PE0), SYSCSR_GPIO_MAP(E, 1, PE1), SYSCSR_GPIO_MAP(E, 2, PE2), SYSCSR_GPIO_MAP(E, 4, PE4),
};

#undef SYSCSR_GPIO_MAP

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static uint32_t syscsr_enter_critical(void);
static void syscsr_exit_critical(uint32_t primask);
static uint8_t syscsr_is_valid_physical_pin(gt32u2086_syscsr_physical_pin_t physical_pin);
static uint8_t syscsr_is_valid_gpio(GPIO_TypeDef const *gpio);
static uint8_t syscsr_is_valid_pad_config(const gt32u2086_syscsr_pad_config_t *config);
static const gt32u2086_syscsr_physical_pad_map_t *syscsr_find_physical_pad(gt32u2086_syscsr_physical_pin_t physical_pin);
static const gt32u2086_syscsr_pin_function_map_t *syscsr_find_function_map(gt32u2086_syscsr_physical_pin_t physical_pin);
static const gt32u2086_syscsr_gpio_map_t *syscsr_find_gpio_map(GPIO_TypeDef const *gpio, uint32_t pin_index);
static neon_status_t syscsr_get_register_set(const gt32u2086_syscsr_physical_pad_map_t *pad,
                                             gt32u2086_syscsr_register_set_t *registers);
static neon_status_t syscsr_resolve_pad(gt32u2086_syscsr_physical_pin_t physical_pin,
                                        const gt32u2086_syscsr_physical_pad_map_t **pad,
                                        gt32u2086_syscsr_register_set_t *registers);
static neon_status_t syscsr_get_register_set_for_edge(neon_syscsr_edge_t edge, gt32u2086_syscsr_register_set_t *registers);
static neon_status_t syscsr_get_mux_value(gt32u2086_syscsr_physical_pin_t physical_pin, gt32u2086_syscsr_pin_function_t function,
                                          uint32_t *mux_value);
static void syscsr_write_mux(const gt32u2086_syscsr_physical_pad_map_t *pad, const gt32u2086_syscsr_register_set_t *registers,
                             uint32_t mux_value);
static void syscsr_write_pad(const gt32u2086_syscsr_physical_pad_map_t *pad, const gt32u2086_syscsr_register_set_t *registers,
                             const gt32u2086_syscsr_pad_config_t *config);
static neon_status_t syscsr_write_boolean_field(volatile uint32_t *reg, uint32_t mask, uint8_t enable);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

void gt32u2086_syscsr_release_pins(gt32u2086_syscsr_pin_group_t *group) {
    uint32_t primask = syscsr_enter_critical();
    for (uint32_t i = group->count; i != 0U; --i) {
        gt32u2086_syscsr_saved_pin_t *saved = &group->pins[i - 1U];
        const gt32u2086_syscsr_physical_pad_map_t *pad;
        gt32u2086_syscsr_register_set_t regs;
        if (syscsr_resolve_pad(saved->pin, &pad, &regs) != NEON_OK) {
            continue;
        }
        uint32_t bit = 1UL << pad->pad_index;
        syscsr_write_mux(pad, &regs, saved->mux);
        *regs.pad_control0 = (*regs.pad_control0 & ~bit) | saved->input_enable;
        *regs.pad_control5 = (*regs.pad_control5 & ~bit) | saved->drive;
        *regs.pad_control6 = (*regs.pad_control6 & ~bit) | saved->open_source;
        *regs.pad_control7 = (*regs.pad_control7 & ~bit) | saved->open_drain;
    }
    group->count = 0U;
    syscsr_exit_critical(primask);
}

neon_status_t gt32u2086_syscsr_configure_pins(gt32u2086_syscsr_pin_group_t *group,
                                       const gt32u2086_syscsr_pin_route_t *routes, uint32_t count, uint32_t open_drain) {
    const gt32u2086_syscsr_physical_pad_map_t *pads[GT32U2086_SYSCSR_MAX_ROUTE_PINS];
    gt32u2086_syscsr_register_set_t regs[GT32U2086_SYSCSR_MAX_ROUTE_PINS];
    uint32_t mux[GT32U2086_SYSCSR_MAX_ROUTE_PINS];
    if (group == NULL || routes == NULL || count == 0U || count > GT32U2086_SYSCSR_MAX_ROUTE_PINS || open_drain > 1U) {
        return NEON_INVALID_PARAM;
    }
    for (uint32_t i = 0U; i < count; ++i) {
        neon_status_t status = syscsr_resolve_pad(routes[i].pin, &pads[i], &regs[i]);
        if (status == NEON_OK) {
            status = syscsr_get_mux_value(routes[i].pin, routes[i].function, &mux[i]);
        }
        if (status != NEON_OK) {
            return status;
        }
        for (uint32_t j = 0U; j < i; ++j) {
            if (routes[i].pin == routes[j].pin) {
                return NEON_INVALID_PARAM;
            }
        }
    }
    uint32_t primask = syscsr_enter_critical();
    uint32_t same = group->count == count;
    for (uint32_t i = 0U; same != 0U && i < count; ++i) {
        same = group->pins[i].pin == routes[i].pin;
    }
    if (same == 0U) {
        gt32u2086_syscsr_release_pins(group);
    }
    for (uint32_t i = 0U; i < count; ++i) {
        uint32_t bit = 1UL << pads[i]->pad_index;
        if (same == 0U) {
            gt32u2086_syscsr_saved_pin_t *saved = &group->pins[i];
            saved->pin = routes[i].pin;
            saved->input_enable = *regs[i].pad_control0 & bit;
            saved->drive = *regs[i].pad_control5 & bit;
            saved->open_source = *regs[i].pad_control6 & bit;
            saved->open_drain = *regs[i].pad_control7 & bit;
            uint32_t index = pads[i]->pad_index;
            saved->mux = index < 16U ? (*regs[i].io_config >> (index * 2U)) & 3U
                                     : (*regs[i].io_config_extended >> ((index - 16U) * 2U)) & 3U;
        }
        *regs[i].pad_control0 |= bit;
        *regs[i].pad_control5 &= ~bit;
        *regs[i].pad_control6 &= ~bit;
        *regs[i].pad_control7 = (*regs[i].pad_control7 & ~bit) | (open_drain != 0U ? bit : 0U);
        syscsr_write_mux(pads[i], &regs[i], mux[i]);
    }
    group->count = count;
    syscsr_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc gt32u2086_syscsr_get_gpio_init_default_pad_config */
void gt32u2086_syscsr_get_gpio_init_default_pad_config(gt32u2086_syscsr_pad_config_t *config) {
    if (config == NULL) {
        return;
    }

    config->input_enable = 1U;
    config->pull = SYSCSR_PULL_NONE;
    config->drive = SYSCSR_DRIVE_2MA;
    config->schmitt_enable = 0U;
    config->slew_rate = SYSCSR_SLEW_SLOW;
}

/** @copydoc neon_syscsr_set_sys_tick_calibration */
neon_status_t neon_syscsr_set_sys_tick_calibration(uint32_t ten_ms_ticks) {
    if (ten_ms_ticks > (SYSCSR_CPU_SYSTICK_TENMS_Msk >> SYSCSR_CPU_SYSTICK_TENMS_Pos)) {
        return NEON_INVALID_PARAM;
    }

    SYSCSR->CPU_SYSTICK = (ten_ms_ticks << SYSCSR_CPU_SYSTICK_TENMS_Pos) & SYSCSR_CPU_SYSTICK_TENMS_Msk;
    return NEON_OK;
}

/** @copydoc neon_syscsr_get_sys_tick_calibration */
uint32_t neon_syscsr_get_sys_tick_calibration(void) {
    return (SYSCSR->CPU_SYSTICK & SYSCSR_CPU_SYSTICK_TENMS_Msk) >> SYSCSR_CPU_SYSTICK_TENMS_Pos;
}

/** @copydoc gt32u2086_syscsr_get_physical_pad_map */
neon_status_t gt32u2086_syscsr_get_physical_pad_map(gt32u2086_syscsr_physical_pin_t physical_pin,
                                              gt32u2086_syscsr_physical_pad_map_t *map) {
    const gt32u2086_syscsr_physical_pad_map_t *found;

    if ((map == NULL) || (syscsr_is_valid_physical_pin(physical_pin) == 0U)) {
        return NEON_INVALID_PARAM;
    }

    found = syscsr_find_physical_pad(physical_pin);
    if (found == NULL) {
        return NEON_NOT_MAPPED;
    }

    *map = *found;
    return NEON_OK;
}

/** @copydoc gt32u2086_syscsr_get_pin_function_map */
neon_status_t gt32u2086_syscsr_get_pin_function_map(gt32u2086_syscsr_physical_pin_t physical_pin,
                                              gt32u2086_syscsr_pin_function_map_t *map) {
    const gt32u2086_syscsr_pin_function_map_t *found;

    if ((map == NULL) || (syscsr_is_valid_physical_pin(physical_pin) == 0U)) {
        return NEON_INVALID_PARAM;
    }

    found = syscsr_find_function_map(physical_pin);
    if (found == NULL) {
        return NEON_NOT_SUPPORTED;
    }

    *map = *found;
    return NEON_OK;
}

/** @copydoc gt32u2086_syscsr_get_gpio_physical_pin */
neon_status_t gt32u2086_syscsr_get_gpio_physical_pin(GPIO_TypeDef *gpio, uint32_t pin_index,
                                               gt32u2086_syscsr_physical_pin_t *physical_pin) {
    const gt32u2086_syscsr_gpio_map_t *map;

    if ((physical_pin == NULL) || (syscsr_is_valid_gpio(gpio) == 0U) || (pin_index >= 8UL)) {
        return NEON_INVALID_PARAM;
    }

    map = syscsr_find_gpio_map(gpio, pin_index);
    if ((map == NULL) || (map->physical_pin == SYSCSR_PIN_INVALID)) {
        return NEON_NOT_SUPPORTED;
    }

    *physical_pin = map->physical_pin;
    return NEON_OK;
}

/** @copydoc gt32u2086_syscsr_set_pin_function */
neon_status_t gt32u2086_syscsr_set_pin_function(gt32u2086_syscsr_physical_pin_t physical_pin, gt32u2086_syscsr_pin_function_t function) {
    const gt32u2086_syscsr_physical_pad_map_t *pad;
    gt32u2086_syscsr_register_set_t registers;
    neon_status_t status;
    uint32_t mux_value;
    uint32_t primask;

    status = syscsr_resolve_pad(physical_pin, &pad, &registers);
    if (status != NEON_OK) {
        return status;
    }

    status = syscsr_get_mux_value(physical_pin, function, &mux_value);
    if (status != NEON_OK) {
        return status;
    }

    primask = syscsr_enter_critical();
    syscsr_write_mux(pad, &registers, mux_value);
    syscsr_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc gt32u2086_syscsr_get_pin_function */
neon_status_t gt32u2086_syscsr_get_pin_function(gt32u2086_syscsr_physical_pin_t physical_pin, gt32u2086_syscsr_pin_function_t *function) {
    const gt32u2086_syscsr_physical_pad_map_t *pad;
    const gt32u2086_syscsr_pin_function_map_t *map;
    gt32u2086_syscsr_register_set_t registers;
    neon_status_t status;
    uint32_t position;
    uint32_t mux_value;

    if (function == NULL) {
        return NEON_INVALID_PARAM;
    }

    status = syscsr_resolve_pad(physical_pin, &pad, &registers);
    if (status != NEON_OK) {
        return status;
    }

    map = syscsr_find_function_map(physical_pin);
    if (map == NULL) {
        return NEON_NOT_SUPPORTED;
    }

    if ((uint32_t)pad->pad_index < 16UL) {
        position = (uint32_t)pad->pad_index * 2UL;
        mux_value = (*registers.io_config >> position) & SYSCSR_MUX_VALUE_MASK;
    } else {
        position = ((uint32_t)pad->pad_index - 16UL) * 2UL;
        mux_value = (*registers.io_config_extended >> position) & SYSCSR_MUX_VALUE_MASK;
    }
    if (map->mux_function[mux_value] == SYSCSR_PIN_FUNCTION_NONE) {
        return NEON_NOT_SUPPORTED;
    }

    *function = map->mux_function[mux_value];
    return NEON_OK;
}

/** @copydoc gt32u2086_syscsr_configure_pad */
neon_status_t gt32u2086_syscsr_configure_pad(gt32u2086_syscsr_physical_pin_t physical_pin, const gt32u2086_syscsr_pad_config_t *config) {
    const gt32u2086_syscsr_physical_pad_map_t *pad;
    gt32u2086_syscsr_register_set_t registers;
    neon_status_t status;
    uint32_t primask;

    if (syscsr_is_valid_pad_config(config) == 0U) {
        return NEON_INVALID_PARAM;
    }

    status = syscsr_resolve_pad(physical_pin, &pad, &registers);
    if (status != NEON_OK) {
        return status;
    }
    if ((uint32_t)pad->pad_index >= SYSCSR_PAD_LEGACY_COUNT) {
        return NEON_NOT_SUPPORTED;
    }

    primask = syscsr_enter_critical();
    syscsr_write_pad(pad, &registers, config);
    syscsr_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc gt32u2086_syscsr_get_pad_config */
neon_status_t gt32u2086_syscsr_get_pad_config(gt32u2086_syscsr_physical_pin_t physical_pin, gt32u2086_syscsr_pad_config_t *config) {
    const gt32u2086_syscsr_physical_pad_map_t *pad;
    gt32u2086_syscsr_register_set_t registers;
    neon_status_t status;
    uint32_t index;
    uint32_t pin_bit;
    uint32_t upper_pin_bit;
    uint32_t ctrl0;
    uint32_t ctrl2;
    uint32_t ctrl3;
    gt32u2086_syscsr_pad_config_t result;

    if (config == NULL) {
        return NEON_INVALID_PARAM;
    }

    status = syscsr_resolve_pad(physical_pin, &pad, &registers);
    if (status != NEON_OK) {
        return status;
    }
    if ((uint32_t)pad->pad_index >= SYSCSR_PAD_LEGACY_COUNT) {
        return NEON_NOT_SUPPORTED;
    }

    index = (uint32_t)pad->pad_index;
    pin_bit = 1UL << index;
    upper_pin_bit = 1UL << (12UL + index);
    ctrl0 = *registers.pad_control0;
    ctrl2 = *registers.pad_control2;
    ctrl3 = *registers.pad_control3;

    result.input_enable = ((ctrl0 & pin_bit) != 0UL) ? 1U : 0U;
    result.drive = (gt32u2086_syscsr_drive_t)((*registers.pad_control4 >> (index * 2UL)) & SYSCSR_PDRV_VALUE_MASK);
    result.schmitt_enable = ((ctrl2 & upper_pin_bit) != 0UL) ? 1U : 0U;
    result.slew_rate = ((ctrl2 & pin_bit) != 0UL) ? SYSCSR_SLEW_FAST : SYSCSR_SLEW_SLOW;

    if ((ctrl0 & upper_pin_bit) != 0UL) {
        result.pull = SYSCSR_PULL_NONE;
    } else if (((ctrl3 & upper_pin_bit) != 0UL) && ((ctrl3 & pin_bit) == 0UL)) {
        result.pull = SYSCSR_PULL_UP;
    } else if (((ctrl3 & pin_bit) != 0UL) && ((ctrl3 & upper_pin_bit) == 0UL)) {
        result.pull = SYSCSR_PULL_DOWN;
    } else {
        return NEON_ERROR;
    }

    *config = result;
    return NEON_OK;
}

/** @copydoc gt32u2086_syscsr_set_pad_coarse_drive */
neon_status_t gt32u2086_syscsr_set_pad_coarse_drive(gt32u2086_syscsr_physical_pin_t physical_pin, gt32u2086_syscsr_coarse_drive_t drive) {
    const gt32u2086_syscsr_physical_pad_map_t *pad;
    gt32u2086_syscsr_register_set_t registers;
    neon_status_t status;
    uint32_t bit;
    uint32_t primask;

    if (drive > SYSCSR_COARSE_DRIVE_HIGH) {
        return NEON_INVALID_PARAM;
    }

    status = syscsr_resolve_pad(physical_pin, &pad, &registers);
    if (status != NEON_OK) {
        return status;
    }
    if ((uint32_t)pad->pad_index >= SYSCSR_PAD_LEGACY_COUNT) {
        return NEON_NOT_SUPPORTED;
    }

    bit = 1UL << (uint32_t)pad->pad_index;
    primask = syscsr_enter_critical();
    if (drive == SYSCSR_COARSE_DRIVE_HIGH) {
        *registers.pad_control1 |= bit;
    } else {
        *registers.pad_control1 &= ~bit;
    }
    syscsr_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc gt32u2086_syscsr_get_pad_coarse_drive */
neon_status_t gt32u2086_syscsr_get_pad_coarse_drive(gt32u2086_syscsr_physical_pin_t physical_pin,
                                              gt32u2086_syscsr_coarse_drive_t *drive) {
    const gt32u2086_syscsr_physical_pad_map_t *pad;
    gt32u2086_syscsr_register_set_t registers;
    neon_status_t status;
    uint32_t bit;

    if (drive == NULL) {
        return NEON_INVALID_PARAM;
    }

    status = syscsr_resolve_pad(physical_pin, &pad, &registers);
    if (status != NEON_OK) {
        return status;
    }
    if ((uint32_t)pad->pad_index >= SYSCSR_PAD_LEGACY_COUNT) {
        return NEON_NOT_SUPPORTED;
    }

    bit = 1UL << (uint32_t)pad->pad_index;
    *drive = ((*registers.pad_control1 & bit) != 0UL) ? SYSCSR_COARSE_DRIVE_HIGH : SYSCSR_COARSE_DRIVE_LOW;
    return NEON_OK;
}

/** @copydoc gt32u2086_syscsr_reset_pin */
neon_status_t gt32u2086_syscsr_reset_pin(gt32u2086_syscsr_physical_pin_t physical_pin) {
    const gt32u2086_syscsr_physical_pad_map_t *pad;
    gt32u2086_syscsr_register_set_t registers;
    neon_status_t status;
    uint32_t index;
    uint32_t pin_bit;
    uint32_t upper_pin_bit;
    uint32_t pdrv_position;
    uint32_t pdrv_mask;
    uint32_t ctrl_pair_mask;
    const gt32u2086_syscsr_pad_reset_t *reset;
    uint32_t primask;

    status = syscsr_resolve_pad(physical_pin, &pad, &registers);
    if (status != NEON_OK) {
        return status;
    }
    if ((uint32_t)pad->pad_index >= SYSCSR_PAD_LEGACY_COUNT) {
        /* The new VP defines mux selectors for all 24 pads. The legacy high-level
     * PAD_CTRL encoding is only known for indexes 0..11, so restore only the
     * unambiguous mux state for the remaining indexes. */
        primask = syscsr_enter_critical();
        syscsr_write_mux(pad, &registers, 0UL);
        syscsr_exit_critical(primask);
        return NEON_OK;
    }

    index = (uint32_t)pad->pad_index;
    pin_bit = 1UL << index;
    upper_pin_bit = 1UL << (12UL + index);
    pdrv_position = index * 2UL;
    pdrv_mask = SYSCSR_PDRV_VALUE_MASK << pdrv_position;
    ctrl_pair_mask = pin_bit | upper_pin_bit;
    reset = &gt32u2086_syscsr_pad_reset[(uint32_t)pad->register_group];

    primask = syscsr_enter_critical();

    /* IO_CFG reset = MUX_0. */
    syscsr_write_mux(pad, &registers, 0UL);

    *registers.pad_control0 = (*registers.pad_control0 & ~ctrl_pair_mask) | (reset->pad_control0 & ctrl_pair_mask);
    *registers.pad_control1 = (*registers.pad_control1 & ~pin_bit) | (reset->pad_control1 & pin_bit);
    *registers.pad_control2 = (*registers.pad_control2 & ~ctrl_pair_mask) | (reset->pad_control2 & ctrl_pair_mask);
    *registers.pad_control3 = (*registers.pad_control3 & ~ctrl_pair_mask) | (reset->pad_control3 & ctrl_pair_mask);
    *registers.pad_control4 = (*registers.pad_control4 & ~pdrv_mask) | (reset->pad_control4 & pdrv_mask);

    syscsr_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc gt32u2086_syscsr_set_gpio_pins_function */
neon_status_t gt32u2086_syscsr_set_gpio_pins_function(GPIO_TypeDef *gpio, uint32_t pin_mask) {
    const gt32u2086_syscsr_physical_pad_map_t *pads[8];
    gt32u2086_syscsr_register_set_t registers[8];
    uint32_t mux_values[8];
    gt32u2086_syscsr_physical_pin_t physical_pin;
    neon_status_t status;
    uint32_t bit;
    uint32_t primask;

    if ((syscsr_is_valid_gpio(gpio) == 0U) || (pin_mask == 0UL) || ((pin_mask & ~SYSCSR_GPIO_PIN_MASK) != 0UL)) {
        return NEON_INVALID_PARAM;
    }

    /* Resolve the entirety before writing so that errors at a pin do not create a bad state. */
    for (bit = 0UL; bit < 8UL; ++bit) {
        if ((pin_mask & (1UL << bit)) == 0UL) {
            continue;
        }

        status = gt32u2086_syscsr_get_gpio_physical_pin(gpio, bit, &physical_pin);
        if (status != NEON_OK) {
            return status;
        }
        status = syscsr_resolve_pad(physical_pin, &pads[bit], &registers[bit]);
        if (status != NEON_OK) {
            return status;
        }
        status = syscsr_get_mux_value(physical_pin, SYSCSR_PIN_FUNCTION_GPIO, &mux_values[bit]);
        if (status != NEON_OK) {
            return status;
        }
    }

    primask = syscsr_enter_critical();
    for (bit = 0UL; bit < 8UL; ++bit) {
        if ((pin_mask & (1UL << bit)) != 0UL) {
            syscsr_write_mux(pads[bit], &registers[bit], mux_values[bit]);
        }
    }
    syscsr_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc gt32u2086_syscsr_configure_gpio_pads */
neon_status_t gt32u2086_syscsr_configure_gpio_pads(GPIO_TypeDef *gpio, uint32_t pin_mask,
                                             const gt32u2086_syscsr_pad_config_t *config) {
    const gt32u2086_syscsr_physical_pad_map_t *pads[8];
    gt32u2086_syscsr_register_set_t registers[8];
    gt32u2086_syscsr_physical_pin_t physical_pin;
    neon_status_t status;
    uint32_t bit;
    uint32_t primask;

    if ((syscsr_is_valid_gpio(gpio) == 0U) || (pin_mask == 0UL) || ((pin_mask & ~SYSCSR_GPIO_PIN_MASK) != 0UL) ||
        (syscsr_is_valid_pad_config(config) == 0U)) {
        return NEON_INVALID_PARAM;
    }

    /* Resolve the entirety before writing so that errors at a pin do not create a bad state. */
    for (bit = 0UL; bit < 8UL; ++bit) {
        if ((pin_mask & (1UL << bit)) == 0UL) {
            continue;
        }

        status = gt32u2086_syscsr_get_gpio_physical_pin(gpio, bit, &physical_pin);
        if (status != NEON_OK) {
            return status;
        }
        status = syscsr_resolve_pad(physical_pin, &pads[bit], &registers[bit]);
        if (status != NEON_OK) {
            return status;
        }
    }

    primask = syscsr_enter_critical();
    for (bit = 0UL; bit < 8UL; ++bit) {
        if (((pin_mask & (1UL << bit)) != 0UL) && ((uint32_t)pads[bit]->pad_index < SYSCSR_PAD_LEGACY_COUNT)) {
            /* PAD_CTRL0..4 only document the legacy 12-pad field encoding.
       * PAD_CTRL5..7 are exposed as raw registers by the new VP, without a
       * complete per-property encoding. Keep high pads at their reset
       * electrical state instead of guessing fields, but do not prevent the
       * GPIO mux/core from being configured. */
            syscsr_write_pad(pads[bit], &registers[bit], config);
        }
    }
    syscsr_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc gt32u2086_syscsr_validate_gpio_pins */
neon_status_t gt32u2086_syscsr_validate_gpio_pins(GPIO_TypeDef *gpio, uint32_t pin_mask) {
    const gt32u2086_syscsr_physical_pad_map_t *pad;
    gt32u2086_syscsr_register_set_t registers;
    gt32u2086_syscsr_physical_pin_t physical_pin;
    neon_status_t status;
    uint32_t mux_value;
    uint32_t bit;

    if ((syscsr_is_valid_gpio(gpio) == 0U) || (pin_mask == 0UL) || ((pin_mask & ~SYSCSR_GPIO_PIN_MASK) != 0UL)) {
        return NEON_INVALID_PARAM;
    }

    for (bit = 0UL; bit < 8UL; ++bit) {
        if ((pin_mask & (1UL << bit)) == 0UL) {
            continue;
        }

        status = gt32u2086_syscsr_get_gpio_physical_pin(gpio, bit, &physical_pin);
        if (status != NEON_OK) {
            return status;
        }
        status = syscsr_resolve_pad(physical_pin, &pad, &registers);
        if (status != NEON_OK) {
            return status;
        }
        status = syscsr_get_mux_value(physical_pin, SYSCSR_PIN_FUNCTION_GPIO, &mux_value);
        if (status != NEON_OK) {
            return status;
        }
    }

    return NEON_OK;
}

/** @copydoc gt32u2086_syscsr_reset_gpio_pins */
neon_status_t gt32u2086_syscsr_reset_gpio_pins(GPIO_TypeDef *gpio, uint32_t pin_mask) {
    gt32u2086_syscsr_physical_pin_t physical_pins[8];
    neon_status_t status;
    uint32_t bit;

    status = gt32u2086_syscsr_validate_gpio_pins(gpio, pin_mask);
    if (status != NEON_OK) {
        return status;
    }

    for (bit = 0UL; bit < 8UL; ++bit) {
        if ((pin_mask & (1UL << bit)) == 0UL) {
            continue;
        }

        status = gt32u2086_syscsr_get_gpio_physical_pin(gpio, bit, &physical_pins[bit]);
        if (status != NEON_OK) {
            return status;
        }
    }

    for (bit = 0UL; bit < 8UL; ++bit) {
        if ((pin_mask & (1UL << bit)) != 0UL) {
            status = gt32u2086_syscsr_reset_pin(physical_pins[bit]);
            if (status != NEON_OK) {
                return status;
            }
        }
    }

    return NEON_OK;
}

/** @copydoc neon_syscsr_get_version */
neon_status_t neon_syscsr_get_version(neon_syscsr_version_t *version) {
    uint32_t raw;

    if (version == NULL) {
        return NEON_INVALID_PARAM;
    }

    raw = SYSCSR->VERSION;
    version->major = (uint16_t)((raw & SYSCSR_VERSION_MAJOR_Msk) >> SYSCSR_VERSION_MAJOR_Pos);
    version->minor = (uint16_t)((raw & SYSCSR_VERSION_MINOR_Msk) >> SYSCSR_VERSION_MINOR_Pos);
    version->raw_value = raw;
    return NEON_OK;
}

/** @copydoc neon_syscsr_get_boot_config */
neon_status_t neon_syscsr_get_boot_config(neon_syscsr_boot_config_t *config) {
    uint32_t raw;

    if (config == NULL) {
        return NEON_INVALID_PARAM;
    }

    raw = SYSCSR->STATUS;
    config->boot_pin = ((raw & SYSCSR_STATUS_BOOT_Msk) != 0UL) ? 1U : 0U;
    config->mode0_pin = ((raw & SYSCSR_STATUS_MODE_0_Msk) != 0UL) ? 1U : 0U;
    config->mode1_pin = ((raw & SYSCSR_STATUS_MODE_1_Msk) != 0UL) ? 1U : 0U;
    config->test_mode_pin = ((raw & SYSCSR_STATUS_TEST_MODE_Msk) != 0UL) ? 1U : 0U;
    config->test_mode_select_pin = ((raw & SYSCSR_STATUS_TEST_MODE_SEL_Msk) != 0UL) ? 1U : 0U;
    config->mode = (uint8_t)(config->mode0_pin | (uint8_t)(config->mode1_pin << 1U));
    config->raw_value = raw;
    return NEON_OK;
}

/** @copydoc neon_syscsr_get_cpu_status */
neon_status_t neon_syscsr_get_cpu_status(neon_syscsr_cpu_status_t *status) {
    uint32_t raw;

    if (status == NULL) {
        return NEON_INVALID_PARAM;
    }

    raw = SYSCSR->CPU_CTRL;
    status->debug_watchdog_freeze = ((raw & SYSCSR_CPU_CTRL_CPU_DEBUG_Msk) != 0UL) ? 1U : 0U;
    status->wic_request = ((raw & SYSCSR_CPU_CTRL_WICENREQ_Msk) != 0UL) ? 1U : 0U;
    status->wic_acknowledge = ((raw & SYSCSR_CPU_CTRL_WICENACK_Msk) != 0UL) ? 1U : 0U;
    status->wakeup = ((raw & SYSCSR_CPU_CTRL_WAKEUP_Msk) != 0UL) ? 1U : 0U;
    status->wic_sense = (uint8_t)((raw & SYSCSR_CPU_CTRL_WICSENSE_Msk) >> SYSCSR_CPU_CTRL_WICSENSE_Pos);
    status->sleep_hold_requested = ((raw & SYSCSR_CPU_CTRL_SLEEPHOLDREQN_Msk) == 0UL) ? 1U : 0U;
    status->sleep_hold_acknowledge = ((raw & SYSCSR_CPU_CTRL_SLEEPHOLDACKN_Msk) == 0UL) ? 1U : 0U;
    status->sleeping = ((raw & SYSCSR_CPU_CTRL_SLEEPING_Msk) != 0UL) ? 1U : 0U;
    status->sleep_deep = ((raw & SYSCSR_CPU_CTRL_SLEEPDEEP_Msk) != 0UL) ? 1U : 0U;
    status->tx_event = ((raw & SYSCSR_CPU_CTRL_TXEV_Msk) != 0UL) ? 1U : 0U;
    status->system_reset_request = ((raw & SYSCSR_CPU_CTRL_SYSRESETREQ_Msk) != 0UL) ? 1U : 0U;
    status->lockup = ((raw & SYSCSR_CPU_CTRL_LOCKUP_Msk) != 0UL) ? 1U : 0U;
    status->raw_value = raw;
    return NEON_OK;
}

/** @copydoc neon_syscsr_set_debug_watchdog_freeze */
neon_status_t neon_syscsr_set_debug_watchdog_freeze(uint8_t enable) {
    return syscsr_write_boolean_field(&SYSCSR->CPU_CTRL, SYSCSR_CPU_CTRL_CPU_DEBUG_Msk, enable);
}

/** @copydoc neon_syscsr_set_wic_request */
neon_status_t neon_syscsr_set_wic_request(uint8_t enable) {
    return syscsr_write_boolean_field(&SYSCSR->CPU_CTRL, SYSCSR_CPU_CTRL_WICENREQ_Msk, enable);
}

/** @copydoc neon_syscsr_set_sleep_hold */
neon_status_t neon_syscsr_set_sleep_hold(uint8_t hold) {
    if (hold > 1U) {
        return NEON_INVALID_PARAM;
    }

    /* Positive low SLEEPHOLDREQN: hold=1 must write field equals 0. */
    return syscsr_write_boolean_field(&SYSCSR->CPU_CTRL, SYSCSR_CPU_CTRL_SLEEPHOLDREQN_Msk, (hold == 0U) ? 1U : 0U);
}

/** @copydoc neon_syscsr_set_oscillator_pad_control */
neon_status_t neon_syscsr_set_oscillator_pad_control(uint8_t control) {
    if (((uint32_t)control & ~SYSCSR_OSC_PAD_CTRL_CONTROL_Msk) != 0UL) {
        return NEON_INVALID_PARAM;
    }

    SYSCSR->OSC_PAD_CTRL = (uint32_t)control;
    return NEON_OK;
}

/** @copydoc neon_syscsr_get_oscillator_pad_control */
uint8_t neon_syscsr_get_oscillator_pad_control(void) {
    return (uint8_t)(SYSCSR->OSC_PAD_CTRL & SYSCSR_OSC_PAD_CTRL_CONTROL_Msk);
}

/** @copydoc neon_syscsr_set_rom_control */
neon_status_t neon_syscsr_set_rom_control(uint8_t enable) {
    return syscsr_write_boolean_field(&SYSCSR->ROM_CTRL, SYSCSR_ROM_CTRL_ENABLE_Msk, enable);
}

/** @copydoc neon_syscsr_get_rom_control */
uint8_t neon_syscsr_get_rom_control(void) {
    return ((SYSCSR->ROM_CTRL & SYSCSR_ROM_CTRL_ENABLE_Msk) != 0UL) ? 1U : 0U;
}

/** @copydoc neon_syscsr_set_extended_pad_control */
neon_status_t neon_syscsr_set_extended_pad_control(neon_syscsr_edge_t edge, uint8_t control_index, uint32_t value) {
    gt32u2086_syscsr_register_set_t registers;
    volatile uint32_t *reg;
    neon_status_t status;

    if ((control_index < 5U) || (control_index > 7U) || ((value & ~SYSCSR_EXT_PAD_MASK) != 0UL)) {
        return NEON_INVALID_PARAM;
    }

    status = syscsr_get_register_set_for_edge(edge, &registers);
    if (status != NEON_OK) {
        return status;
    }

    reg = (control_index == 5U) ? registers.pad_control5
                                : ((control_index == 6U) ? registers.pad_control6 : registers.pad_control7);
    *reg = value;
    return NEON_OK;
}

/** @copydoc neon_syscsr_get_extended_pad_control */
neon_status_t neon_syscsr_get_extended_pad_control(neon_syscsr_edge_t edge, uint8_t control_index, uint32_t *value) {
    gt32u2086_syscsr_register_set_t registers;
    volatile uint32_t *reg;
    neon_status_t status;

    if ((value == NULL) || (control_index < 5U) || (control_index > 7U)) {
        return NEON_INVALID_PARAM;
    }

    status = syscsr_get_register_set_for_edge(edge, &registers);
    if (status != NEON_OK) {
        return status;
    }

    reg = (control_index == 5U) ? registers.pad_control5
                                : ((control_index == 6U) ? registers.pad_control6 : registers.pad_control7);
    *value = *reg & SYSCSR_EXT_PAD_MASK;
    return NEON_OK;
}

/** @copydoc neon_syscsr_set_extended_io_config */
neon_status_t neon_syscsr_set_extended_io_config(neon_syscsr_edge_t edge, uint16_t value) {
    gt32u2086_syscsr_register_set_t registers;
    neon_status_t status = syscsr_get_register_set_for_edge(edge, &registers);

    if (status != NEON_OK) {
        return status;
    }

    *registers.io_config_extended = (uint32_t)value;
    return NEON_OK;
}

/** @copydoc neon_syscsr_get_extended_io_config */
neon_status_t neon_syscsr_get_extended_io_config(neon_syscsr_edge_t edge, uint16_t *value) {
    gt32u2086_syscsr_register_set_t registers;
    neon_status_t status;

    if (value == NULL) {
        return NEON_INVALID_PARAM;
    }

    status = syscsr_get_register_set_for_edge(edge, &registers);
    if (status != NEON_OK) {
        return status;
    }

    const uint32_t result_word = *registers.io_config_extended & SYSCSR_EXT_IO_MASK;
    memcpy(value, &result_word, 2U);
    return NEON_OK;
}

/** @copydoc neon_syscsr_set_jtag_mux */
neon_status_t neon_syscsr_set_jtag_mux(uint8_t enable) {
    return syscsr_write_boolean_field(&SYSCSR->SYS_CTRL, SYSCSR_SYS_CTRL_JTAGMUX_SEL_Msk, enable);
}

/** @copydoc neon_syscsr_is_jtag_mux_enabled */
uint8_t neon_syscsr_is_jtag_mux_enabled(void) {
    return ((SYSCSR->SYS_CTRL & SYSCSR_SYS_CTRL_JTAGMUX_SEL_Msk) != 0UL) ? 1U : 0U;
}

/** @copydoc neon_syscsr_is_jtag_mux_blocked */
uint8_t neon_syscsr_is_jtag_mux_blocked(void) {
    return ((SYSCSR->SYS_CTRL & SYSCSR_SYS_CTRL_JTAGMUX_BLOCK_Msk) != 0UL) ? 1U : 0U;
}

/** @copydoc neon_syscsr_block_jtag_mux */
neon_status_t neon_syscsr_block_jtag_mux(void) {
    return neon_syscsr_set_jtag_mux_blocked(1U);
}

/** @copydoc neon_syscsr_set_jtag_mux_blocked */
neon_status_t neon_syscsr_set_jtag_mux_blocked(uint8_t blocked) {
    return syscsr_write_boolean_field(&SYSCSR->SYS_CTRL, SYSCSR_SYS_CTRL_JTAGMUX_BLOCK_Msk, blocked);
}

/** @copydoc neon_syscsr_request_subsystem_reset * @details
 * GT32U2086 v8 resets FLASHC but not the CPU on SW_RESET_REQ. This entire routine,
 * including its literal pool, must execute from SRAM until FLASH INIT is done.
 */
__attribute__((section(".ramfunc.syscsr_reset"), noinline)) neon_status_t neon_syscsr_request_subsystem_reset(void) {
    uint32_t primask = __get_PRIMASK();
    uint32_t attempt;
    uint32_t poll;

    __disable_irq();
    SYSCSR->SYS_CTRL = SYSCSR_SYS_CTRL_SW_RESET_REQ_Msk;

    for (attempt = 0UL; attempt < SYSCSR_RESET_ATTEMPTS; ++attempt) {
        FLASH_CTRL->COMMAND = SYSCSR_FLASH_INIT_CMD;
        for (poll = 0UL; poll < SYSCSR_RESET_POLLS; ++poll) {
            if (((FLASH_CTRL->COMMAND & FLASH_COMMAND_START_Msk) == 0UL) &&
                ((FLASH_CTRL->STATUS & FLASH_STATUS_INIT_DONE_Msk) != 0UL)) {
                __set_PRIMASK(primask);
                return NEON_OK;
            }
        }
    }

    __set_PRIMASK(primask);
    return NEON_TIMEOUT;
}

/** @copydoc neon_syscsr_request_software_reset */
neon_status_t neon_syscsr_request_software_reset(void) {
    return neon_syscsr_request_subsystem_reset();
}

/** @copydoc neon_syscsr_request_global_reset */
__NO_RETURN void neon_syscsr_request_global_reset(void) {
    __disable_irq();
    SYSCSR->SYS_CTRL |= SYSCSR_SYS_CTRL_SW_GLOBAL_RST_REQ_Msk;
    __DSB();
    for (;;) {
        __WFI();
    }
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

static uint32_t syscsr_enter_critical(void) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void syscsr_exit_critical(uint32_t primask) {
    __set_PRIMASK(primask);
}

static uint8_t syscsr_is_valid_physical_pin(gt32u2086_syscsr_physical_pin_t physical_pin) {
    return ((physical_pin >= SYSCSR_PIN_1) && (physical_pin <= SYSCSR_PIN_80)) ? 1U : 0U;
}

static uint8_t syscsr_is_valid_gpio(GPIO_TypeDef const *gpio) {
    return ((gpio == GPIOA) || (gpio == GPIOB) || (gpio == GPIOC) || (gpio == GPIOD) || (gpio == GPIOE) ||
            (gpio == GPIOF))
               ? 1U
               : 0U;
}

static uint8_t syscsr_is_valid_pad_config(const gt32u2086_syscsr_pad_config_t *config) {
    if (config == NULL) {
        return 0U;
    }

    return (((config->input_enable == 0U) || (config->input_enable == 1U)) && (config->pull <= SYSCSR_PULL_DOWN) &&
            (config->drive <= SYSCSR_DRIVE_12MA) &&
            ((config->schmitt_enable == 0U) || (config->schmitt_enable == 1U)) &&
            (config->slew_rate <= SYSCSR_SLEW_FAST))
               ? 1U
               : 0U;
}

static const gt32u2086_syscsr_physical_pad_map_t *syscsr_find_physical_pad(gt32u2086_syscsr_physical_pin_t physical_pin) {
    size_t index;

    for (index = 0U; index < (sizeof(gt32u2086_syscsr_physical_pad_map) / sizeof(gt32u2086_syscsr_physical_pad_map[0])); ++index) {
        if (gt32u2086_syscsr_physical_pad_map[index].physical_pin == physical_pin) {
            return &gt32u2086_syscsr_physical_pad_map[index];
        }
    }

    return NULL;
}

static const gt32u2086_syscsr_pin_function_map_t *syscsr_find_function_map(gt32u2086_syscsr_physical_pin_t physical_pin) {
    size_t index;

    for (index = 0U; index < (sizeof(gt32u2086_syscsr_function_map) / sizeof(gt32u2086_syscsr_function_map[0])); ++index) {
        if (gt32u2086_syscsr_function_map[index].physical_pin == physical_pin) {
            return &gt32u2086_syscsr_function_map[index];
        }
    }

    return NULL;
}

static const gt32u2086_syscsr_gpio_map_t *syscsr_find_gpio_map(GPIO_TypeDef const *gpio, uint32_t pin_index) {
    size_t index;

    for (index = 0U; index < (sizeof(gt32u2086_syscsr_gpio_map) / sizeof(gt32u2086_syscsr_gpio_map[0])); ++index) {
        if ((gt32u2086_syscsr_gpio_map[index].gpio_port == gpio) &&
            ((uint32_t)gt32u2086_syscsr_gpio_map[index].gpio_pin_index == pin_index)) {
            return &gt32u2086_syscsr_gpio_map[index];
        }
    }

    return NULL;
}

static neon_status_t syscsr_get_register_set(const gt32u2086_syscsr_physical_pad_map_t *pad,
                                             gt32u2086_syscsr_register_set_t *registers) {
    if ((pad == NULL) || (registers == NULL)) {
        return NEON_INVALID_PARAM;
    }

    switch (pad->register_group) {
        case SYSCSR_REGISTER_GROUP_LEFT:
            registers->io_config = &SYSCSR->LEFT_IO_CFG;
            registers->pad_control0 = &SYSCSR->LEFT_PAD_CTRL0;
            registers->pad_control1 = &SYSCSR->LEFT_PAD_CTRL1;
            registers->pad_control2 = &SYSCSR->LEFT_PAD_CTRL2;
            registers->pad_control3 = &SYSCSR->LEFT_PAD_CTRL3;
            registers->pad_control4 = &SYSCSR->LEFT_PAD_CTRL4;
            registers->pad_control5 = &SYSCSR->LEFT_PAD_CTRL5;
            registers->pad_control6 = &SYSCSR->LEFT_PAD_CTRL6;
            registers->pad_control7 = &SYSCSR->LEFT_PAD_CTRL7;
            registers->io_config_extended = &SYSCSR->LEFT_IO_CFG_EXT;
            break;

        case SYSCSR_REGISTER_GROUP_BOTTOM:
            registers->io_config = &SYSCSR->BOT_IO_CFG;
            registers->pad_control0 = &SYSCSR->BOT_PAD_CTRL0;
            registers->pad_control1 = &SYSCSR->BOT_PAD_CTRL1;
            registers->pad_control2 = &SYSCSR->BOT_PAD_CTRL2;
            registers->pad_control3 = &SYSCSR->BOT_PAD_CTRL3;
            registers->pad_control4 = &SYSCSR->BOT_PAD_CTRL4;
            registers->pad_control5 = &SYSCSR->BOT_PAD_CTRL5;
            registers->pad_control6 = &SYSCSR->BOT_PAD_CTRL6;
            registers->pad_control7 = &SYSCSR->BOT_PAD_CTRL7;
            registers->io_config_extended = &SYSCSR->BOT_IO_CFG_EXT;
            break;

        case SYSCSR_REGISTER_GROUP_RIGHT:
            registers->io_config = &SYSCSR->RIGHT_IO_CFG;
            registers->pad_control0 = &SYSCSR->RIGHT_PAD_CTRL0;
            registers->pad_control1 = &SYSCSR->RIGHT_PAD_CTRL1;
            registers->pad_control2 = &SYSCSR->RIGHT_PAD_CTRL2;
            registers->pad_control3 = &SYSCSR->RIGHT_PAD_CTRL3;
            registers->pad_control4 = &SYSCSR->RIGHT_PAD_CTRL4;
            registers->pad_control5 = &SYSCSR->RIGHT_PAD_CTRL5;
            registers->pad_control6 = &SYSCSR->RIGHT_PAD_CTRL6;
            registers->pad_control7 = &SYSCSR->RIGHT_PAD_CTRL7;
            registers->io_config_extended = &SYSCSR->RIGHT_IO_CFG_EXT;
            break;

        case SYSCSR_REGISTER_GROUP_TOP:
            registers->io_config = &SYSCSR->TOP_IO_CFG;
            registers->pad_control0 = &SYSCSR->TOP_PAD_CTRL0;
            registers->pad_control1 = &SYSCSR->TOP_PAD_CTRL1;
            registers->pad_control2 = &SYSCSR->TOP_PAD_CTRL2;
            registers->pad_control3 = &SYSCSR->TOP_PAD_CTRL3;
            registers->pad_control4 = &SYSCSR->TOP_PAD_CTRL4;
            registers->pad_control5 = &SYSCSR->TOP_PAD_CTRL5;
            registers->pad_control6 = &SYSCSR->TOP_PAD_CTRL6;
            registers->pad_control7 = &SYSCSR->TOP_PAD_CTRL7;
            registers->io_config_extended = &SYSCSR->TOP_IO_CFG_EXT;
            break;

        default:
            return NEON_NOT_MAPPED;
    }

    return NEON_OK;
}

static neon_status_t syscsr_resolve_pad(gt32u2086_syscsr_physical_pin_t physical_pin,
                                        const gt32u2086_syscsr_physical_pad_map_t **pad,
                                        gt32u2086_syscsr_register_set_t *registers) {
    if ((pad == NULL) || (registers == NULL) || (syscsr_is_valid_physical_pin(physical_pin) == 0U)) {
        return NEON_INVALID_PARAM;
    }

    *pad = syscsr_find_physical_pad(physical_pin);
    if (*pad == NULL) {
        return NEON_NOT_MAPPED;
    }

    return syscsr_get_register_set(*pad, registers);
}

static neon_status_t syscsr_get_register_set_for_edge(neon_syscsr_edge_t edge, gt32u2086_syscsr_register_set_t *registers) {
    gt32u2086_syscsr_physical_pad_map_t pad;

    if ((registers == NULL) || ((uint32_t)edge > (uint32_t)NEON_SYSCSR_EDGE_TOP)) {
        return NEON_INVALID_PARAM;
    }

    pad.physical_pin = SYSCSR_PIN_INVALID;
    pad.register_group = (gt32u2086_syscsr_register_group_t)edge;
    pad.pad_index = 0U;
    return syscsr_get_register_set(&pad, registers);
}

static neon_status_t syscsr_get_mux_value(gt32u2086_syscsr_physical_pin_t physical_pin, gt32u2086_syscsr_pin_function_t function,
                                          uint32_t *mux_value) {
    const gt32u2086_syscsr_pin_function_map_t *map;
    uint32_t mux;

    if ((mux_value == NULL) || (function <= SYSCSR_PIN_FUNCTION_NONE) || (function >= SYSCSR_PIN_FUNCTION_COUNT)) {
        return NEON_INVALID_PARAM;
    }

    map = syscsr_find_function_map(physical_pin);
    if (map == NULL) {
        return NEON_NOT_SUPPORTED;
    }

    for (mux = 0UL; mux <= SYSCSR_MUX_VALUE_MASK; ++mux) {
        if (map->mux_function[mux] == function) {
            *mux_value = mux;
            return NEON_OK;
        }
    }

    return NEON_NOT_SUPPORTED;
}

static void syscsr_write_mux(const gt32u2086_syscsr_physical_pad_map_t *pad, const gt32u2086_syscsr_register_set_t *registers,
                             uint32_t mux_value) {
    volatile uint32_t *io_config;
    uint32_t mux_index = (uint32_t)pad->pad_index;
    uint32_t position;
    uint32_t mask;
    uint32_t value;

    /* VP stores the 48-bit edge mux as 32 low bits plus 16 extension bits. */
    if (mux_index < 16UL) {
        io_config = registers->io_config;
        position = mux_index * 2UL;
    } else {
        io_config = registers->io_config_extended;
        position = (mux_index - 16UL) * 2UL;
    }
    mask = SYSCSR_MUX_VALUE_MASK << position;
    value = *io_config;

    value = (value & ~mask) | ((mux_value & SYSCSR_MUX_VALUE_MASK) << position);
    *io_config = value;
}

static void syscsr_write_pad(const gt32u2086_syscsr_physical_pad_map_t *pad, const gt32u2086_syscsr_register_set_t *registers,
                             const gt32u2086_syscsr_pad_config_t *config) {
    uint32_t index = (uint32_t)pad->pad_index;
    uint32_t pin_bit = 1UL << index;
    uint32_t upper_pin_bit = 1UL << (12UL + index);
    uint32_t ctrl0 = *registers->pad_control0;
    uint32_t ctrl2 = *registers->pad_control2;
    uint32_t ctrl3 = *registers->pad_control3;
    uint32_t ctrl4 = *registers->pad_control4;
    uint32_t pdrv_position = index * 2UL;
    uint32_t pdrv_mask = SYSCSR_PDRV_VALUE_MASK << pdrv_position;

    if (config->input_enable != 0U) {
        ctrl0 |= pin_bit;
    } else {
        ctrl0 &= ~pin_bit;
    }

    ctrl3 &= ~(pin_bit | upper_pin_bit);
    if (config->pull == SYSCSR_PULL_NONE) {
        ctrl0 |= upper_pin_bit; /* PE=1: pull cell does not work. */
    } else {
        ctrl0 &= ~upper_pin_bit; /* PE=0: active pull cell. */
        if (config->pull == SYSCSR_PULL_UP) {
            ctrl3 |= upper_pin_bit;
        } else {
            ctrl3 |= pin_bit;
        }
    }

    if (config->slew_rate == SYSCSR_SLEW_FAST) {
        ctrl2 |= pin_bit;
    } else {
        ctrl2 &= ~pin_bit;
    }

    if (config->schmitt_enable != 0U) {
        ctrl2 |= upper_pin_bit;
    } else {
        ctrl2 &= ~upper_pin_bit;
    }

    ctrl4 = (ctrl4 & ~pdrv_mask) | (((uint32_t)config->drive & SYSCSR_PDRV_VALUE_MASK) << pdrv_position);

    *registers->pad_control0 = ctrl0;
    *registers->pad_control2 = ctrl2;
    *registers->pad_control3 = ctrl3;
    *registers->pad_control4 = ctrl4;
}

static neon_status_t syscsr_write_boolean_field(volatile uint32_t *reg, uint32_t mask, uint8_t enable) {
    uint32_t primask;

    if ((reg == NULL) || (enable > 1U)) {
        return NEON_INVALID_PARAM;
    }

    primask = syscsr_enter_critical();
    if (enable != 0U) {
        *reg |= mask;
    } else {
        *reg &= ~mask;
    }
    syscsr_exit_critical(primask);
    return NEON_OK;
}
