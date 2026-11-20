/**
 * @file neon_phy.c
 * @brief Generic PHY interface: lookup, reference counts and mode matching, the same for every PHY and chip.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "phy/neon_phy.h"
#include "phy/neon_phy_platform.h"
#include <stddef.h>
#include <string.h>

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/
neon_status_t neon_phy_get(const char *name, neon_phy_t *phy)
{
    const neon_phy_platform_entry_t *entry;
    uint32_t i;

    if ((name == NULL) || (phy == NULL)) {
        return NEON_INVALID_PARAM;
    }
    for (i = 0U; i < neon_phy_platform_count; i++) {
        entry = &neon_phy_platform_table[i];
        if (strcmp(entry->name, name) == 0) {
            (void)memset(phy, 0, sizeof(*phy));
            phy->ops = entry->ops;
            phy->priv = entry->priv;
            phy->mode = entry->ops->mode;
            phy->submode = entry->ops->submode;
            return NEON_OK;
        }
    }
    return NEON_NOT_MAPPED;
}

neon_status_t neon_phy_init(neon_phy_t *phy)
{
    neon_status_t status = NEON_OK;

    if ((phy == NULL) || (phy->ops == NULL)) {
        return NEON_INVALID_PARAM;
    }
    if (phy->init_count == UINT8_MAX) {
        return NEON_ERROR;
    }
    if (phy->init_count == 0U) {
        (void)memset(phy->backend_words, 0, sizeof(phy->backend_words));
        if (phy->ops->init != NULL) {
            status = phy->ops->init(phy);
        }
    }
    if (status == NEON_OK) {
        phy->init_count++;
    }
    return status;
}

neon_status_t neon_phy_exit(neon_phy_t *phy)
{
    neon_status_t status = NEON_OK;

    if ((phy == NULL) || (phy->ops == NULL)) {
        return NEON_INVALID_PARAM;
    }
    if (phy->init_count == 0U) {
        return NEON_ERROR;
    }
    if (phy->power_count != 0U) {
        return NEON_BUSY;
    }
    if ((phy->init_count == 1U) && (phy->ops->exit != NULL)) {
        status = phy->ops->exit(phy);
    }
    /* Counted down even when the driver failed: the client gives the PHY up either way. */
    phy->init_count--;
    return status;
}

neon_status_t neon_phy_set_mode(neon_phy_t *phy, neon_phy_mode_t mode, int32_t submode)
{
    neon_status_t status;

    if ((phy == NULL) || (phy->ops == NULL) || (mode == NEON_PHY_MODE_INVALID)) {
        return NEON_INVALID_PARAM;
    }
    if (phy->init_count == 0U) {
        return NEON_ERROR;
    }
    if (phy->power_count != 0U) {
        return NEON_BUSY;
    }
    if (phy->ops->set_mode == NULL) {
        return ((mode == phy->mode) && (submode == phy->submode)) ? NEON_OK : NEON_NOT_SUPPORTED;
    }
    status = phy->ops->set_mode(phy, mode, submode);
    if (status == NEON_OK) {
        phy->mode = mode;
        phy->submode = submode;
    }
    return status;
}

neon_status_t neon_phy_validate(const neon_phy_t *phy, neon_phy_mode_t mode, const neon_phy_configure_opts_t *opts)
{
    if ((phy == NULL) || (phy->ops == NULL) || (opts == NULL)) {
        return NEON_INVALID_PARAM;
    }
    if ((mode != phy->mode) || (phy->ops->validate == NULL)) {
        return NEON_NOT_SUPPORTED;
    }
    return phy->ops->validate(phy, opts);
}

neon_status_t neon_phy_configure(neon_phy_t *phy, neon_phy_mode_t mode, const neon_phy_configure_opts_t *opts)
{
    neon_status_t status;

    if ((phy == NULL) || (phy->ops == NULL) || (opts == NULL)) {
        return NEON_INVALID_PARAM;
    }
    if (phy->init_count == 0U) {
        return NEON_ERROR;
    }
    status = neon_phy_validate(phy, mode, opts);
    if (status != NEON_OK) {
        return status;
    }
    if (phy->ops->configure == NULL) {
        return NEON_NOT_SUPPORTED;
    }
    return phy->ops->configure(phy, opts);
}

neon_status_t neon_phy_power_on(neon_phy_t *phy)
{
    neon_status_t status = NEON_OK;

    if ((phy == NULL) || (phy->ops == NULL)) {
        return NEON_INVALID_PARAM;
    }
    if ((phy->init_count == 0U) || (phy->power_count == UINT8_MAX)) {
        return NEON_ERROR;
    }
    if ((phy->power_count == 0U) && (phy->ops->power_on != NULL)) {
        status = phy->ops->power_on(phy);
    }
    if (status == NEON_OK) {
        phy->power_count++;
    }
    return status;
}

neon_status_t neon_phy_power_off(neon_phy_t *phy)
{
    neon_status_t status = NEON_OK;

    if ((phy == NULL) || (phy->ops == NULL)) {
        return NEON_INVALID_PARAM;
    }
    if (phy->power_count == 0U) {
        return NEON_ERROR;
    }
    if ((phy->power_count == 1U) && (phy->ops->power_off != NULL)) {
        status = phy->ops->power_off(phy);
    }
    /* Counted down even when the driver failed: the client has stopped using the link either way. */
    phy->power_count--;
    return status;
}

neon_status_t neon_phy_take_status(neon_phy_t *phy, neon_phy_mode_t mode, neon_phy_status_t *status)
{
    if ((phy == NULL) || (phy->ops == NULL) || (status == NULL)) {
        return NEON_INVALID_PARAM;
    }
    if (phy->init_count == 0U) {
        return NEON_ERROR;
    }
    if ((mode != phy->mode) || (phy->ops->take_status == NULL)) {
        return NEON_NOT_SUPPORTED;
    }
    return phy->ops->take_status(phy, status);
}
