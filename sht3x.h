// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#ifndef __SHT3X_H
#define __SHT3X_H

#include "main.h"
#include <stdbool.h>

#define SHT3X_ADDRESS_DEFAULT        ((uint16_t)0x44)
#define SHT3X_ADDRESS_VDD            ((uint16_t)0x45)

#define SHT3X_CLOCK_STRETCH_HIGH     (0x062c)
#define SHT3X_CLOCK_STRETCH_MEDIUM   (0x0d2c)
#define SHT3X_CLOCK_STRETCH_LOW      (0x102c)

#ifdef __cplusplus
#define EXPORT extern "C"
#else
#define EXPORT
#endif


// Checks that SHT3x sensor is present on the bus.
// NOTE: that it will only work property after sensor soft/hard reset.
// Params:
//  - `hi2c`: I2C bus
//  - `address`: device address, either SHT3X_ADDRESS_DEFAULT or SHT3X_ADDRESS_VDD
// Returns true if sensor responded
EXPORT bool sht3x_sensor_present(I2C_HandleTypeDef *hi2c, uint16_t address);

// Performs one shot measurement
//  - `hi2c`: I2C bus
//  - `address`: device address, either SHT3X_ADDRESS_DEFAULT or SHT3X_ADDRESS_VDD
//  - `temp`: output - where to save temperature, in C, multiplied by 100, e.g.
//     23.5C -> 2350
//  - `hum`: output - where to save humidity, in percents (0-100%)
// Returns true in case of successful measurement, false otherwise
EXPORT bool sht3x_one_shot_measurement_clock_stretching(I2C_HandleTypeDef *hi2c, uint16_t address, uint16_t type, int32_t* temp, uint32_t* hum);

#endif
