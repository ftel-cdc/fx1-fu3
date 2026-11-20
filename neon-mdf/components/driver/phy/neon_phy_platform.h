/**
 * @file neon_phy_platform.h
 * @brief @internal What a chip family supplies to the generic PHY layer: the table of its PHYs.
 * @details
 * neon_phy.c holds every rule of neon_phy.h and is the same for every chip. A family defines the table
 * below in phy/platform/<family>/neon_phy_<family>.c, one entry per PHY instance; two instances of the same
 * PHY share their ops and differ by priv. Applications must not include this file.
 */

#ifndef __NEON_PHY_PLATFORM_H__
#define __NEON_PHY_PLATFORM_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "phy/neon_phy.h"
#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
/** One PHY instance of the chip. */
typedef struct {
    const char *name;                          /* Name given to neon_phy_get(). */
    const neon_phy_ops_t *ops;                 /* Driver of the instance. */
    const void *priv;                          /* Instance data handed to the driver as phy->priv; may be NULL. */
} neon_phy_platform_entry_t;

/*==================================================================================================
*                                      GLOBAL VARIABLES
==================================================================================================*/
/* PHY instances of the chip, defined by the family. */
extern const neon_phy_platform_entry_t neon_phy_platform_table[];
/* Entries in neon_phy_platform_table. */
extern const uint32_t neon_phy_platform_count;

#ifdef __cplusplus
}
#endif

#endif /* __NEON_PHY_PLATFORM_H__ */
