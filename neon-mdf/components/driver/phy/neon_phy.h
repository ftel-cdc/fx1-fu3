/**
 * @file neon_phy.h
 * @brief Generic PHY interface, after the Linux generic PHY framework (include/linux/phy/phy.h).
 * @details
 * A PHY is the physical layer under a controller: a MIPI D-PHY under a CSI-2 receiver or a DSI host, and
 * later a USB, PCIe or combo PHY. The controller driver is the client: it looks its PHY up by name, then
 * drives it with the calls below:
 *
 *   neon_phy_get() -> neon_phy_init() -> [neon_phy_set_mode()] -> [neon_phy_configure()] -> neon_phy_power_on()
 *   ... in use: neon_phy_take_status(), [neon_phy_configure()] ...
 *   neon_phy_power_off() -> neon_phy_exit()
 *
 * This file and neon_phy.c hold only what is true for every PHY: the lookup by name, the counting of init
 * and power_on, and the match between the mode of a call and the mode of the PHY. Every other rule (when a
 * setting may change, which settings are accepted, what a power-on does) belongs to the PHY driver in
 * phy/platform/<family>/, and is documented in phy/platform/<family>/neon_phy_<family>.h. Steps that a chip
 * puts in the registers of the controller (resets, lane enables) stay in the controller's driver.
 * @note init and power_on count their calls, so that two clients may share one PHY: the driver is called
 * by the first neon_phy_init() and the first neon_phy_power_on(), and by the last exit or power_off.
 */

#ifndef __NEON_PHY_H__
#define __NEON_PHY_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stdbool.h>
#include <stdint.h>
#include "neon_types.h"
#include "phy/neon_phy_mipi_dphy.h"
#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
/* Words of storage inside the handle, private to the PHY driver. */
#define NEON_PHY_BACKEND_WORDS 8U

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
/** Kind of link a PHY carries: selects the member of neon_phy_configure_opts_t and neon_phy_status_t. */
typedef enum {
    NEON_PHY_MODE_INVALID = 0,    /* Not set. */
    NEON_PHY_MODE_MIPI_DPHY = 1   /* MIPI D-PHY: opts.mipi_dphy, status.mipi_dphy; submodes in neon_phy_mipi_dphy.h. */
} neon_phy_mode_t;

/** Settings of neon_phy_configure(), one member per mode. */
typedef union {
    neon_phy_mipi_dphy_opts_t mipi_dphy;          /* NEON_PHY_MODE_MIPI_DPHY. */
} neon_phy_configure_opts_t;

/** Result of neon_phy_take_status(), one member per mode. */
typedef union {
    neon_phy_mipi_dphy_status_t mipi_dphy;        /* NEON_PHY_MODE_MIPI_DPHY. */
} neon_phy_status_t;

typedef struct neon_phy_s neon_phy_t;

/**
 * Operations of one PHY driver. An operation left NULL does nothing and succeeds, except validate,
 * configure and take_status, which then return NEON_NOT_SUPPORTED, and set_mode, which then accepts only
 * the mode and submode the PHY already has.
 */
typedef struct {
    neon_phy_mode_t mode;                                                                     /* Mode the PHY starts in after neon_phy_get(). */
    int32_t submode;                                                                          /* Submode it starts in; meaning per mode. */
    neon_status_t (*init)(neon_phy_t *phy);                                                   /* First neon_phy_init(). */
    neon_status_t (*exit)(neon_phy_t *phy);                                                   /* Last neon_phy_exit(). */
    neon_status_t (*set_mode)(neon_phy_t *phy, neon_phy_mode_t mode, int32_t submode);        /* Change the mode; the core records it on success. */
    neon_status_t (*validate)(const neon_phy_t *phy, const neon_phy_configure_opts_t *opts);  /* Settings this PHY can take in its mode; no hardware. */
    neon_status_t (*configure)(neon_phy_t *phy, const neon_phy_configure_opts_t *opts);       /* Program validated settings. */
    neon_status_t (*power_on)(neon_phy_t *phy);                                               /* First neon_phy_power_on(). */
    neon_status_t (*power_off)(neon_phy_t *phy);                                              /* Last neon_phy_power_off(). */
    neon_status_t (*take_status)(neon_phy_t *phy, neon_phy_status_t *status);                 /* Read and clear the sticky errors. */
} neon_phy_ops_t;

/** PHY instance, caller allocated. Filled by neon_phy_get(); fields are read by the PHY driver, written by this module. */
struct neon_phy_s {
    const neon_phy_ops_t *ops;                       /* Driver of this PHY. */
    const void *priv;                                /* Instance data of the driver (register base, index), from the platform table. */
    neon_phy_mode_t mode;                            /* Current mode. */
    int32_t submode;                                 /* Current submode. */
    uint8_t init_count;                              /* neon_phy_init() calls not yet matched by neon_phy_exit(). */
    uint8_t power_count;                             /* neon_phy_power_on() calls not yet matched by neon_phy_power_off(). */
    uint64_t backend_words[NEON_PHY_BACKEND_WORDS];  /* Private to the PHY driver; cleared by the first neon_phy_init(). */
};

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Bind a handle to the PHY of that name on this chip. Touches no hardware.
 * @param[in] name PHY name, listed in phy/platform/<family>/neon_phy_<family>.h.
 * @param[out] phy PHY handle, overwritten. It must not be in use.
 * @return NEON_OK, NEON_INVALID_PARAM for a NULL pointer, or NEON_NOT_MAPPED if the chip has no PHY of that name.
 */
neon_status_t neon_phy_get(const char *name, neon_phy_t *phy);

/**
 * @brief Set the PHY up. Only the first call reaches the driver.
 * @param[in,out] phy PHY handle from neon_phy_get().
 * @return NEON_OK, NEON_INVALID_PARAM for a NULL or unbound handle, NEON_ERROR when the count is full,
 * or a driver status. A refused first call is not counted.
 */
neon_status_t neon_phy_init(neon_phy_t *phy);

/**
 * @brief Undo one neon_phy_init(). The last call reaches the driver.
 * @param[in,out] phy PHY handle.
 * @return NEON_OK, NEON_INVALID_PARAM for a NULL handle, NEON_ERROR if not initialized, NEON_BUSY while
 * powered, or a driver status (the call is counted anyway).
 */
neon_status_t neon_phy_exit(neon_phy_t *phy);

/**
 * @brief Select what the PHY carries, e.g. D-PHY receive or transmit, or one protocol of a combo PHY.
 * @param[in,out] phy PHY handle, initialized and not powered.
 * @param[in] mode Mode.
 * @param[in] submode Submode of that mode (neon_phy_mipi_dphy.h for D-PHY).
 * @return NEON_OK, NEON_INVALID_PARAM for a NULL handle or NEON_PHY_MODE_INVALID, NEON_ERROR if not
 * initialized, NEON_BUSY while powered, NEON_NOT_SUPPORTED for a mode the PHY does not have, or a driver status.
 */
neon_status_t neon_phy_set_mode(neon_phy_t *phy, neon_phy_mode_t mode, int32_t submode);

/**
 * @brief Check settings without applying them. Touches no hardware.
 * @param[in] phy PHY handle, bound.
 * @param[in] mode Mode of the settings; must be the PHY's current mode.
 * @param[in] opts Settings, the member of that mode.
 * @return NEON_OK, NEON_INVALID_PARAM for a NULL pointer or a value the PHY cannot take, NEON_NOT_SUPPORTED
 * for another mode or a setting outside what the PHY provides.
 */
neon_status_t neon_phy_validate(const neon_phy_t *phy, neon_phy_mode_t mode, const neon_phy_configure_opts_t *opts);

/**
 * @brief Validate then program settings. Whether this is allowed while powered is the driver's rule.
 * @param[in,out] phy PHY handle, initialized.
 * @param[in] mode Mode of the settings; must be the PHY's current mode.
 * @param[in] opts Settings.
 * @return NEON_OK, NEON_ERROR if not initialized, a status of neon_phy_validate(), or a driver status.
 */
neon_status_t neon_phy_configure(neon_phy_t *phy, neon_phy_mode_t mode, const neon_phy_configure_opts_t *opts);

/**
 * @brief Power the PHY up. Only the first call reaches the driver.
 * @param[in,out] phy PHY handle, initialized. Whether it must be configured first is the driver's rule.
 * @return NEON_OK, NEON_INVALID_PARAM for a NULL handle, NEON_ERROR if not initialized or the count is
 * full, or a driver status. A refused first call is not counted.
 */
neon_status_t neon_phy_power_on(neon_phy_t *phy);

/**
 * @brief Undo one neon_phy_power_on(). The last call reaches the driver.
 * @param[in,out] phy PHY handle.
 * @return NEON_OK, NEON_INVALID_PARAM for a NULL handle, NEON_ERROR if not powered, or a driver status
 * (the call is counted anyway).
 */
neon_status_t neon_phy_power_off(neon_phy_t *phy);

/**
 * @brief Read the errors the PHY recorded since the previous call, and clear exactly those read.
 * @param[in,out] phy PHY handle, initialized.
 * @param[in] mode Mode of the result; must be the PHY's current mode.
 * @param[out] status Errors, the member of that mode.
 * @return NEON_OK, NEON_INVALID_PARAM for a NULL pointer, NEON_ERROR if not initialized, NEON_NOT_SUPPORTED
 * for another mode, or a driver status.
 */
neon_status_t neon_phy_take_status(neon_phy_t *phy, neon_phy_mode_t mode, neon_phy_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_PHY_H__ */
