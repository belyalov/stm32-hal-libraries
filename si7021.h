// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#ifndef __SI7021_H
#define __SI7021_H

#include "main.h"

// Build in heater commands: on/off.
// Use it with si7021_set_config()
#define SI7021_HEATER_ON                 (0x1 << 2)
#define SI7021_HEATER_OFF                0

// Temperature / humidity measure resolution
// Use it with si7021_set_config()
#define SI7021_RESOLUTION_RH12_TEMP14    0      // Hum 12 bits, temp 14 bits
#define SI7021_RESOLUTION_RH8_TEMP12     1      // and so on
#define SI7021_RESOLUTION_RH10_TEMP13    (1 << 7)
#define SI7021_RESOLUTION_RH11_TEMP11    (1 << 7 | 1)

// Selectable build in heater power
// Use it with si7021_set_heater_power()
#define SI7021_HEATER_POWER_3MA          0
#define SI7021_HEATER_POWER_9MA          0x1
#define SI7021_HEATER_POWER_15MA         0x2
#define SI7021_HEATER_POWER_27MA         0x4
#define SI7021_HEATER_POWER_51MA         0x8
#define SI7021_HEATER_POWER_94MA         0xf

// In case of something went wrong...
#define SI7021_MEASURE_FAILED            0xFFFF

#ifdef __cplusplus
#define EXPORT extern "C"
#else
#define EXPORT
#endif


// The si7021 provides a serial number individualized for each device
// Params:
//  - `hi2c` I2C bus
// Returns device id or 0 in case of error.
EXPORT uint64_t si7021_read_id(I2C_HandleTypeDef *hi2c);

// Turns on/off build-in heater / changes measurement resolution
// Params:
//  - `hi2c` I2C bus
//  - `heater` - build in heater mode. One of SI7021_HEATER_ON / SI7021_HEATER_OFF
//  - `resolution` - measurement resolution. One of SI7021_RESOLUTION_*
// Returns 0 (HAL_OK) or error.
EXPORT uint32_t si7021_set_config(I2C_HandleTypeDef *hi2c, uint8_t heater, uint8_t resolution);

// Sets build in heater power current
// Params:
//  - `hi2c` I2C bus
//  - `power` - one of SI7021_HEATER_POWER_*
// Returns 0 (HAL_OK) or error.
EXPORT uint32_t si7021_set_heater_power(I2C_HandleTypeDef *hi2c, uint8_t power);

// Starts humidity measurement. As a side effect temperature will be measure as well.
// To get temperature value use `si7021_read_previous_temperature()`
// Params:
//  - `hi2c` I2C bus
// Returns current humidity, e.g. 55
// In case of error returns `SI7021_MEASURE_FAILED`
EXPORT uint32_t si7021_measure_humidity(I2C_HandleTypeDef *hi2c);

// Starts temperature measurement. This blocking call takes about 20ms to complete.
// Params:
//  - `hi2c` I2C bus
// Returns current temperature in Celsius multiplied by 100 (to avoid float integers)
// e.g. `+23.12C` -> `2312`
// In case of error returns `SI7021_MEASURE_FAILED`
EXPORT int32_t si7021_measure_temperature(I2C_HandleTypeDef *hi2c);

// Reads previous temperature measurement done by prior call of `si7021_measure_humidity()`
// Params:
//  - `hi2c` I2C bus
// Returns current temperature in Celsius multiplied by 100 (to avoid float integers)
// e.g. `+23.12C` -> `2312`
// In case of error returns `SI7021_MEASURE_FAILED`
EXPORT int32_t si7021_read_previous_temperature(I2C_HandleTypeDef *hi2c);

#endif
