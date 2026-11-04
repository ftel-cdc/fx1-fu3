/**
 * @file neon_rtc.c
 * @brief Implementation for RTC.
 * @details
 * Implements module behavior and target-specific integration.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "rtc/neon_rtc.h"

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
/* bit location/true mask (inc_fu2/fu2_dti_rtc.h). */
#define RTC_CPSR_DIV_MASK 0x3FFFFFFU
#define RTC_CPSR_RTC_EN (1U << 31)

#define RTC_TIMECFG_SECOND_POS 0U
#define RTC_TIMECFG_SECOND_MASK (0x7FU << RTC_TIMECFG_SECOND_POS)
#define RTC_TIMECFG_MINUTE_POS 7U
#define RTC_TIMECFG_MINUTE_MASK (0x7FU << RTC_TIMECFG_MINUTE_POS)
#define RTC_TIMECFG_HOUR_POS 14U
#define RTC_TIMECFG_HOUR_MASK (0x3FU << RTC_TIMECFG_HOUR_POS)

#define RTC_RTIME_MILLISECOND_MASK 0xFFU
#define RTC_RTIME_SECOND_POS 8U
#define RTC_RTIME_SECOND_MASK (0x7FU << RTC_RTIME_SECOND_POS)
#define RTC_RTIME_MINUTE_POS 15U
#define RTC_RTIME_MINUTE_MASK (0x7FU << RTC_RTIME_MINUTE_POS)
#define RTC_RTIME_HOUR_POS 22U
#define RTC_RTIME_HOUR_MASK (0x3FU << RTC_RTIME_HOUR_POS)

#define RTC_DATECFG_DATE_POS 0U
#define RTC_DATECFG_DATE_MASK (0x3FU << RTC_DATECFG_DATE_POS)
#define RTC_DATECFG_MONTH_POS 6U
#define RTC_DATECFG_MONTH_MASK (0x1FU << RTC_DATECFG_MONTH_POS)
#define RTC_DATECFG_YEAR_POS 11U
#define RTC_DATECFG_YEAR_MASK (0xFFFFU << RTC_DATECFG_YEAR_POS)

#define RTC_RDATE_DATE_POS 0U
#define RTC_RDATE_DATE_MASK (0x3FU << RTC_RDATE_DATE_POS)
#define RTC_RDATE_DAY_POS 6U
#define RTC_RDATE_DAY_MASK (0xFU << RTC_RDATE_DAY_POS)
#define RTC_RDATE_MONTH_POS 10U
#define RTC_RDATE_MONTH_MASK (0x1FU << RTC_RDATE_MONTH_POS)
#define RTC_RDATE_YEAR_POS 15U
#define RTC_RDATE_YEAR_MASK (0xFFFFU << RTC_RDATE_YEAR_POS)

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static uint32_t rtc_bcd_to_decimal(uint32_t value);
static uint32_t rtc_decimal_to_bcd(uint32_t value);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_rtc_init */
neon_status_t neon_rtc_init(RTC_TypeDef *instance, const neon_rtc_config_t *init) {
    uint32_t prescaler;

    if (instance != RTC || init == 0) {
        return NEON_INVALID_PARAM;
    }

    prescaler = (init->clock_frequency_hz != 0) ? (init->clock_frequency_hz / 100U) : init->prescaler;
    if (prescaler > RTC_CPSR_DIV_MASK) {
        return NEON_INVALID_PARAM;
    }

    /* write prescaler with RTC_EN=0 first (pre-setup), then enable RTC_EN - in the correct order according
   * to fu2_dti_rtc.c::LL_RTC_Init (Fig.11 in original document). */
    instance->CPSR = prescaler & RTC_CPSR_DIV_MASK;
    instance->CPSR |= RTC_CPSR_RTC_EN;

    return NEON_OK;
}

/** @copydoc neon_rtc_deinit */
neon_status_t neon_rtc_deinit(RTC_TypeDef *instance) {
    if (instance != RTC) {
        return NEON_INVALID_PARAM;
    }

    instance->CPSR &= ~RTC_CPSR_RTC_EN; /* stop counter first */
    instance->TIMECFG = 0;
    instance->DATECFG = 0;
    instance->CPSR = 0;
    instance->ALAR0 = 0;
    instance->ALAR1_INS0 = 0;
    instance->ALAR1_INS1 = 0;
    instance->INTR_INS0 = 0;
    instance->INTR_INS1 = 0;
    instance->CSTW = 0;
    instance->RSTW = 0;
    instance->CTSTW = 0;

    return NEON_OK;
}

/** @copydoc neon_rtc_set_time */
neon_status_t neon_rtc_set_time(RTC_TypeDef *instance, const neon_rtc_time_t *time) {
    if (instance != RTC || time == 0 || time->hour > 23 || time->minute > 59 || time->second > 59) {
        return NEON_INVALID_PARAM;
    }

    /* write TIMECFG activates hardware to automatically latch new hours in 1 period clock - matches
   * fu2_dti_rtc.h::LL_RTC_TIME_Set. */
    instance->TIMECFG = ((rtc_decimal_to_bcd(time->hour) << RTC_TIMECFG_HOUR_POS) & RTC_TIMECFG_HOUR_MASK) |
                        ((rtc_decimal_to_bcd(time->minute) << RTC_TIMECFG_MINUTE_POS) & RTC_TIMECFG_MINUTE_MASK) |
                        ((rtc_decimal_to_bcd(time->second) << RTC_TIMECFG_SECOND_POS) & RTC_TIMECFG_SECOND_MASK);

    return NEON_OK;
}

/** @copydoc neon_rtc_get_time */
neon_status_t neon_rtc_get_time(RTC_TypeDef *instance, neon_rtc_time_t *time) {
    uint32_t raw;

    if (instance != RTC || time == 0) {
        return NEON_INVALID_PARAM;
    }

    raw = instance->RTIME;
    time->hour = (uint8_t)rtc_bcd_to_decimal((raw & RTC_RTIME_HOUR_MASK) >> RTC_RTIME_HOUR_POS);
    time->minute = (uint8_t)rtc_bcd_to_decimal((raw & RTC_RTIME_MINUTE_MASK) >> RTC_RTIME_MINUTE_POS);
    time->second = (uint8_t)rtc_bcd_to_decimal((raw & RTC_RTIME_SECOND_MASK) >> RTC_RTIME_SECOND_POS);

    return NEON_OK;
}

/** @copydoc neon_rtc_set_date */
neon_status_t neon_rtc_set_date(RTC_TypeDef *instance, const neon_rtc_date_t *date) {
    static const uint32_t days_in_month[12] = {31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U};
    uint32_t days;

    if (instance != RTC || date == 0 || date->year < 1U || date->year > 9999U || date->month < 1U ||
        date->month > 12U || date->date < 1U) {
        return NEON_INVALID_PARAM;
    }
    days = days_in_month[date->month - 1U];
    if (date->month == 2U && (date->year % 4U) == 0U && ((date->year % 100U) != 0U || (date->year % 400U) == 0U)) {
        days = 29U;
    }
    if (date->date > days) {
        return NEON_INVALID_PARAM;
    }

    /* day-of-week NOT writeable - hardware autocomputes when writing DATECFG, matching
   * fu2_dti_rtc.h::LL_RTC_DATE_Set (write note "day is auto-computed"). */
    instance->DATECFG = ((rtc_decimal_to_bcd(date->year) << RTC_DATECFG_YEAR_POS) & RTC_DATECFG_YEAR_MASK) |
                        ((rtc_decimal_to_bcd(date->month) << RTC_DATECFG_MONTH_POS) & RTC_DATECFG_MONTH_MASK) |
                        ((rtc_decimal_to_bcd(date->date) << RTC_DATECFG_DATE_POS) & RTC_DATECFG_DATE_MASK);

    return NEON_OK;
}

/** @copydoc neon_rtc_get_date */
neon_status_t neon_rtc_get_date(RTC_TypeDef *instance, neon_rtc_date_t *date) {
    uint32_t raw;

    if (instance != RTC || date == 0) {
        return NEON_INVALID_PARAM;
    }

    raw = instance->RDATE;
    date->year = (uint16_t)rtc_bcd_to_decimal((raw & RTC_RDATE_YEAR_MASK) >> RTC_RDATE_YEAR_POS);
    date->month = (uint8_t)rtc_bcd_to_decimal((raw & RTC_RDATE_MONTH_MASK) >> RTC_RDATE_MONTH_POS);
    date->date = (uint8_t)rtc_bcd_to_decimal((raw & RTC_RDATE_DATE_MASK) >> RTC_RDATE_DATE_POS);
    date->day = (uint8_t)((raw & RTC_RDATE_DAY_MASK) >> RTC_RDATE_DAY_POS); /* day NOT BCD according to original header */

    return NEON_OK;
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

/* The year occupies four BCD digits; time fields use the same helpers. */
static uint32_t rtc_bcd_to_decimal(uint32_t value) {
    uint32_t result = 0U;
    uint32_t multiplier = 1U;
    while (value != 0U) {
        result += (value & 0xFU) * multiplier;
        multiplier *= 10U;
        value >>= 4U;
    }
    return result;
}

static uint32_t rtc_decimal_to_bcd(uint32_t value) {
    uint32_t result = 0U;
    uint32_t shift = 0U;
    while (value != 0U) {
        result |= (value % 10U) << shift;
        value /= 10U;
        shift += 4U;
    }
    return result;
}
