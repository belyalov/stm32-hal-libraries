// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#ifndef __SHT4X_H
#define __SHT4X_H

#include "main.h"
#include <stdbool.h>

// Unlike SHT3x, SHT4x has no address pin - I2C address is fixed by part number:
//   SHT4x-AD1B -> 0x44, SHT4x-BD1B -> 0x45, SHT4x-CD1B -> 0x46
#define SHT4X_ADDRESS_A              ((uint16_t)0x44)
#define SHT4X_ADDRESS_B              ((uint16_t)0x45)
#define SHT4X_ADDRESS_C              ((uint16_t)0x46)

// Measurement precision - the higher precision the longer measurement takes
#define SHT4X_PRECISION_HIGH         (0xFD)
#define SHT4X_PRECISION_MEDIUM       (0xF6)
#define SHT4X_PRECISION_LOW          (0xE0)

#ifdef __cplusplus
#define EXPORT extern "C"
#else
#define EXPORT
#endif


// Reads unique serial number of the sensor.
// Params:
//  - `hi2c`: I2C bus
//  - `address`: device address, one of SHT4X_ADDRESS_A / _B / _C
//  - `serial`: output - where to save serial number, may be NULL
// Returns true if sensor responded with CRC valid data
EXPORT bool sht4x_read_serial(I2C_HandleTypeDef *hi2c, uint16_t address, uint32_t* serial);

// Checks that SHT4x sensor is present on the bus.
// Params:
//  - `hi2c`: I2C bus
//  - `address`: device address, one of SHT4X_ADDRESS_A / _B / _C
// Returns true if sensor responded
EXPORT bool sht4x_sensor_present(I2C_HandleTypeDef *hi2c, uint16_t address);

// Performs one shot measurement.
// SHT4x does not support clock stretching, so call blocks (HAL_Delay) for the
// duration of the measurement, which depends on requested precision.
//  - `hi2c`: I2C bus
//  - `address`: device address, one of SHT4X_ADDRESS_A / _B / _C
//  - `precision`: one of SHT4X_PRECISION_HIGH / _MEDIUM / _LOW
//  - `temp`: output - where to save temperature, in C, multiplied by 100, e.g.
//     23.5C -> 2350
//  - `hum`: output - where to save humidity, in percents (0-100%)
// Returns true in case of successful measurement, false otherwise
EXPORT bool sht4x_one_shot_measurement(I2C_HandleTypeDef *hi2c, uint16_t address, uint8_t precision, int32_t* temp, uint32_t* hum);

#endif
