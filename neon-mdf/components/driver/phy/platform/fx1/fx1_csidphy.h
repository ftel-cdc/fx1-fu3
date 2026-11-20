/**
 * @file fx1_csidphy.h
 * @brief @internal Operations and instance data of csi-dphy0, for the FX1 table of PHYs (neon_phy_fx1.c).
 * @details Applications must not include this file: they reach the PHY through neon_phy_get().
 */

#ifndef __FX1_CSIDPHY_H__
#define __FX1_CSIDPHY_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "phy/platform/fx1/neon_phy_fx1.h"
#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
/** Instance data of csi-dphy0, given as phy->priv by the FX1 table of PHYs. */
typedef struct {
    uintptr_t base;                            /* APB base of the register block; 0: the IP has none (timers tied at integration). */
} fx1_csidphy_instance_t;

/*==================================================================================================
*                                      GLOBAL VARIABLES
==================================================================================================*/
/* Operations of csi-dphy0, defined in fx1_csidphy.c. */
extern const neon_phy_ops_t fx1_csidphy_ops;

#ifdef __cplusplus
}
#endif

#endif /* __FX1_CSIDPHY_H__ */
