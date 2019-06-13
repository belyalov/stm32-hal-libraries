// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#ifndef __VEML6030_H
#define __VEML6030_H

#include "main.h"

#ifdef __cplusplus
#define EXPORT extern "C"
#else
#define EXPORT
#endif

// ALS gain constants
#define REG_ALS_CONF_GAIN_1     (0x00 << 11) // x1 (default)
#define REG_ALS_CONF_GAIN_2     (0x01 << 11) // x2
#define REG_ALS_CONF_GAIN_1_8   (0x02 << 11) // x(1/8)
#define REG_ALS_CONF_GAIN_1_4   (0x03 << 11) // x(1/4)

// ALS integration times (ms)
#define REG_ALS_CONF_IT25       (0x0C << 6)
#define REG_ALS_CONF_IT50       (0x08 << 6)
#define REG_ALS_CONF_IT100      (0x00 << 6)
#define REG_ALS_CONF_IT200      (0x01 << 6)
#define REG_ALS_CONF_IT400      (0x02 << 6)
#define REG_ALS_CONF_IT800      (0x03 << 6)


enum {
  VEML6030_OK = 0,
  VEML6030_ERROR = 1,
};

typedef struct {
  I2C_HandleTypeDef *i2c;
  uint8_t            read_addr;
  uint8_t            write_addr;
} veml6030;

// Initialize VEML6030 sensor.
// Params:
//  - `veml`: VEML6030 definition to be initialized
//  - `i2c` I2C HAL bus (`hi2c1`, `hi2c2`, etc)
//  - `addr` VEML6030 address. Either 0x10 or 0x48
// Returns:
//  - `VEML6030_OK` - device initialized successfully
//  - `VEML6030_ERROR` - initialization failed (e.g. device not found on the i2c bus)
EXPORT uint32_t veml6030_init(veml6030 *veml, I2C_HandleTypeDef *i2c, uint8_t addr);

// Power control //

// Power on veml6030
// Params:
//  - `veml` initialized instance
// Returns:
//  - `VEML6030_OK` - device powered on
//  - `VEML6030_ERROR` - I2C error
EXPORT uint32_t veml6030_power_on(veml6030 *veml);

// Shutdown veml6030 (make it sleep)
// Params:
//  - `veml` initialized instance
// Returns:
//  - `VEML6030_OK` - device put into sleep
//  - `VEML6030_ERROR` - I2C error
EXPORT uint32_t veml6030_shutdown(veml6030 *veml);

// ALS integration time configuration //

// Set Integration Time
// Params:
//  - `veml` initialized instance
//  - it - anything from REG_ALS_CONF_IT*
// Returns:
//  - `VEML6030_OK` - device put into sleep
//  - `VEML6030_ERROR` - I2C error
EXPORT uint32_t veml6030_set_als_integration_time(veml6030 *veml, uint16_t it);

// Get current Integration Time
// Params:
//  - `veml` initialized instance
// Returns current integration time
EXPORT uint16_t veml6030_get_als_integration_time(veml6030 *veml);

// ALS gain configuration //

// Set ALS gain
// Params:
//  - `veml` initialized instance
//  - gain - anything from REG_ALS_CONF_GAIN_*
// Returns:
//  - `VEML6030_OK` - device put into sleep
//  - `VEML6030_ERROR` - I2C error
EXPORT uint32_t veml6030_set_als_gain(veml6030 *veml, uint16_t gain);

// Get current ALS gain value
// Params:
//  - `veml` initialized instance
// Returns current gain value (REG_ALS_CONF_GAIN_*)
EXPORT uint16_t veml6030_get_als_gain(veml6030 *veml);

// Read current sensor values //

// Read previous measurement of ALS
// Params:
//  - `veml` initialized instance
// Returns current ALS
EXPORT uint16_t veml6030_read_als(veml6030 *veml);

// Read previous measurement of WHITE
// Params:
//  - `veml` initialized instance
// Returns current WHITE value
EXPORT uint16_t veml6030_read_white(veml6030 *veml);

#endif
