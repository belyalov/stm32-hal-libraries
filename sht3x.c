// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#include "sht3x.h"


#define READ_ADDR(addr)    ((addr << 1) | 0x01)
#define WRITE_ADDR(addr)    (addr << 1)


static uint32_t convert_temperature(uint16_t raw)
{
    // Formula for Celsius: -45 + 175 * (raw / 0xffff)
    int32_t result = raw * 175 * 100;
    result /= 0xFFFF;
    result -= 4500;
    return result;
}

static uint32_t convert_humidity(uint16_t raw)
{
    // Formula: 100 * (raw / 0xffff)
    uint32_t result = raw * 100;
    result /= 0xFFFF;
    return result;
}

static uint8_t gen_crc8(uint8_t *data, uint32_t len)
{
    uint8_t crc = 0xff;  // CRC init 0xff

    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint32_t j = 0; j < 8; j++) {
            if ((crc & 0x80) != 0)
                crc = (uint8_t)((crc << 1) ^ 0x31);   // Poly 0x31
            else
                crc <<= 1;
        }
    }
    return crc;
}

bool sht3x_sensor_present(I2C_HandleTypeDef *hi2c, uint16_t address)
{
    // Read Status Register
    uint8_t buf[2] = {0xF3, 0x2D};

    int res = HAL_I2C_Master_Transmit(hi2c, WRITE_ADDR(address), buf, 2, 1000);
    if (res != HAL_OK) {
        return false;
    }

    res = HAL_I2C_Master_Receive(hi2c, READ_ADDR(address), buf, 2, 1000);
    if (res != HAL_OK) {
        return false;
    }

    return buf[0] == 0x80 && buf[1] == 0x10;
}

bool sht3x_one_shot_measurement_clock_stretching(I2C_HandleTypeDef *hi2c, uint16_t address, uint16_t type, int32_t* temp, uint32_t* hum)
{
    int res = HAL_I2C_Master_Transmit(hi2c, WRITE_ADDR(address), (uint8_t*)&type, 2, 1000);
    if (res != HAL_OK) {
        return false;
    }

    uint8_t buf[6];
    res = HAL_I2C_Master_Receive(hi2c, READ_ADDR(address), buf, 6, 1000);
    if (res != HAL_OK) {
        return false;
    }

    // Ensure data correctness
    if (gen_crc8(buf, 2) != buf[2]) {
        return false;
    }
    if (gen_crc8(&buf[3], 2) != buf[5]) {
        return false;
    }

    // Convert:
    // - Temp to Celsius * 100
    // - Hum to relative percents
    uint16_t raw_temp = buf[0] << 8 | buf[1];
    uint16_t raw_hum = buf[3] << 8 | buf[4];

    if (temp) {
        *temp = convert_temperature(raw_temp);
    }
    if (hum) {
        *hum = convert_humidity(raw_hum);
    }

    return true;
}
