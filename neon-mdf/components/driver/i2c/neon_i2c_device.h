/** @file neon_i2c_device.h
 * @brief Device-specific I2C instance aliases.
 */
#ifndef __NEON_I2C_DEVICE_H__
#define __NEON_I2C_DEVICE_H__
#include "mcu_device.h"
typedef I2C_TypeDef *neon_i2c_t;
#if defined(I2C0)
#define NEON_I2C0 I2C0
#endif
#endif
