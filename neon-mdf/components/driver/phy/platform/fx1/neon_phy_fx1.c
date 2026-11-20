/**
 * @file neon_phy_fx1.c
 * @brief The PHY instances of FX1.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "phy/platform/fx1/neon_phy_fx1.h"
#include "phy/neon_phy_platform.h"
#include "phy/platform/fx1/fx1_csidphy.h"

/*==================================================================================================
*                                        LOCAL MACROS
==================================================================================================*/
/* The D-PHY register block has no address yet (OPEN K3): 0 until one is given at build time. */
#ifdef FX1_CSIDPHY_BASE
#define FX1_CSIDPHY0_BASE ((uintptr_t)(FX1_CSIDPHY_BASE))
#else
#define FX1_CSIDPHY0_BASE ((uintptr_t)0U)
#endif

/*==================================================================================================
*                                      LOCAL VARIABLES
==================================================================================================*/
static const fx1_csidphy_instance_t fx1_csidphy0 = {
    .base = FX1_CSIDPHY0_BASE,                 /* 0: no register block. */
};

/*==================================================================================================
*                                      GLOBAL VARIABLES
==================================================================================================*/
const neon_phy_platform_entry_t neon_phy_platform_table[] = {
    {NEON_PHY_FX1_CSI_DPHY0, &fx1_csidphy_ops, &fx1_csidphy0},
};

const uint32_t neon_phy_platform_count = (uint32_t)(sizeof(neon_phy_platform_table) / sizeof(neon_phy_platform_table[0]));
