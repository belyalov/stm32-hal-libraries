// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#ifndef __SHTC3_H
#define __SHTC3_H

#include "main.h"

#ifdef __cplusplus
#define EXPORT extern "C"
#else
#define EXPORT
#endif


// The shtc3 provides a serial number individualized for each device
// Params:
//  - `hi2c` I2C bus
// Returns device id or 0 in case of error.
EXPORT uint16_t shtc3_read_id(I2C_HandleTypeDef *hi2c);

// Put sensor into sleep mode
// Params:
//  - `hi2c` I2C bus
// Returns device id or 0 in case of error.
EXPORT uint32_t shtc3_sleep(I2C_HandleTypeDef *hi2c);

// Wake up sensor.
// You must wait for 240us to let sensor enter into IDLE mode.
// Params:
//  - `hi2c` I2C bus
// Returns zero in case of error
EXPORT uint32_t shtc3_wakeup(I2C_HandleTypeDef *hi2c);

// Performs full cycle: starts temperature/humidity measurements using "clock stretch" method.
// Params:
//  - `hi2c` I2C bus
//  - `temp` measured temperature, in C multiplied by 100 (e.g. 24.1C -> 2410)
//  - `hum` measured relative humidity, in percents
// Returns zero in case of error
EXPORT uint32_t shtc3_perform_measurements(I2C_HandleTypeDef *hi2c, int32_t* temp, int32_t* hum);

// Start temperature/humidity measurements using "clock stretch" approach, in low power mode.
// After completed - values can be obtained by shtc3_read_measurements()
// Params:
//  - `hi2c` I2C bus
//  - `temp` measured temperature, in C multiplied by 100 (e.g. 24.1C -> 2410)
//  - `hum` measured relative humidity, in percents
// Returns zero in case of error
EXPORT uint32_t shtc3_perform_measurements_low_power(I2C_HandleTypeDef *hi2c, int32_t* out_temp, int32_t* out_hum);

#endif
