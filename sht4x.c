// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#include "sht4x.h"


#define READ_ADDR(addr)    ((addr << 1) | 0x01)
#define WRITE_ADDR(addr)    (addr << 1)

#define SHT4X_CMD_READ_SERIAL    (0x89)

// SHT4x has no clock stretching, so master must wait until measurement
// is completed. Datasheet max durations (8.3 / 4.5 / 1.6 ms), rounded up.
#define SHT4X_DELAY_HIGH         (10)
#define SHT4X_DELAY_MEDIUM       (5)
#define SHT4X_DELAY_LOW          (2)
#define SHT4X_DELAY_SERIAL       (10)


static int32_t convert_temperature(uint16_t raw)
{
    // Formula for Celsius: -45 + 175 * (raw / 0xffff)
    int32_t result = raw * 175 * 100;
    result /= 0xFFFF;
    result -= 4500;
    return result;
}

static uint32_t convert_humidity(uint16_t raw)
{
    // Formula: -6 + 125 * (raw / 0xffff)
    // NOTE: unlike SHT3x, result may fall outside of 0-100% and must be clipped
    int32_t result = raw * 125;
    result /= 0xFFFF;
    result -= 6;

    if (result < 0) {
        return 0;
    }
    if (result > 100) {
        return 100;
    }
    return result;
}

static uint32_t measurement_delay(uint8_t precision)
{
    switch (precision) {
    case SHT4X_PRECISION_LOW:
        return SHT4X_DELAY_LOW;
    case SHT4X_PRECISION_MEDIUM:
        return SHT4X_DELAY_MEDIUM;
    default:
        return SHT4X_DELAY_HIGH;
    }
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

// Reads 2 words + CRC each, i.e. exactly what any SHT4x command replies with
static bool read_words(I2C_HandleTypeDef *hi2c, uint16_t address, uint16_t* word1, uint16_t* word2)
{
    uint8_t buf[6];

    int res = HAL_I2C_Master_Receive(hi2c, READ_ADDR(address), buf, 6, 1000);
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

    *word1 = buf[0] << 8 | buf[1];
    *word2 = buf[3] << 8 | buf[4];

    return true;
}

bool sht4x_read_serial(I2C_HandleTypeDef *hi2c, uint16_t address, uint32_t* serial)
{
    uint8_t cmd = SHT4X_CMD_READ_SERIAL;

    int res = HAL_I2C_Master_Transmit(hi2c, WRITE_ADDR(address), &cmd, 1, 1000);
    if (res != HAL_OK) {
        return false;
    }

    HAL_Delay(SHT4X_DELAY_SERIAL);

    uint16_t hi, lo;
    if (!read_words(hi2c, address, &hi, &lo)) {
        return false;
    }

    if (serial) {
        *serial = (uint32_t)hi << 16 | lo;
    }

    return true;
}

bool sht4x_sensor_present(I2C_HandleTypeDef *hi2c, uint16_t address)
{
    // SHT4x has no status register, so serial number is used to detect sensor
    return sht4x_read_serial(hi2c, address, NULL);
}

bool sht4x_one_shot_measurement(I2C_HandleTypeDef *hi2c, uint16_t address, uint8_t precision, int32_t* temp, uint32_t* hum)
{
    int res = HAL_I2C_Master_Transmit(hi2c, WRITE_ADDR(address), &precision, 1, 1000);
    if (res != HAL_OK) {
        return false;
    }

    HAL_Delay(measurement_delay(precision));

    // Convert:
    // - Temp to Celsius * 100
    // - Hum to relative percents
    uint16_t raw_temp, raw_hum;
    if (!read_words(hi2c, address, &raw_temp, &raw_hum)) {
        return false;
    }

    if (temp) {
        *temp = convert_temperature(raw_temp);
    }
    if (hum) {
        *hum = convert_humidity(raw_hum);
    }

    return true;
}
