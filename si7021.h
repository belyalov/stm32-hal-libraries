// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#ifndef __SI7021_H
#define __SI7021_H

#include "main.h"

#define SI7021_HEATER_ON                 (0x1 << 2)
#define SI7021_HEATER_OFF                0

#define SI7021_RESOLUTION_RH12_TEMP14    0
#define SI7021_RESOLUTION_RH8_TEMP12     1
#define SI7021_RESOLUTION_RH10_TEMP13    (1 << 7)
#define SI7021_RESOLUTION_RH11_TEMP11    (1 << 7 | 1)

#define SI7021_HEATER_POWER_3MA          0
#define SI7021_HEATER_POWER_9MA          0x1
#define SI7021_HEATER_POWER_15MA         0x2
#define SI7021_HEATER_POWER_27MA         0x4
#define SI7021_HEATER_POWER_51MA         0x8
#define SI7021_HEATER_POWER_94MA         0xf

#define SI7021_MEASURE_FAILED            1024


uint64_t si7021_read_id(I2C_HandleTypeDef *hi2c);
uint32_t si7021_set_config(I2C_HandleTypeDef *hi2c, uint8_t heater, uint8_t resolution);
uint32_t si7021_set_heater_power(I2C_HandleTypeDef *hi2c, uint8_t power);
uint32_t si7021_measure_humidity(I2C_HandleTypeDef *hi2c);
uint32_t si7021_measure_temperature(I2C_HandleTypeDef *hi2c);
uint32_t si7021_read_previous_temperature(I2C_HandleTypeDef *hi2c);

#endif
