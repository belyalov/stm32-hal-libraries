// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#include <gtest/gtest.h>

#include "sht4x.h"
#include "test_mocks.h"

using namespace std;


class sht4x : public ::testing::Test {
protected:
  void SetUp() override {
    I2C_clear_transmit_history();
    I2C_clear_transmit_queue();
  }

  I2C_HandleTypeDef i2c;
};

TEST_F(sht4x, serial)
{
  // 2 words, each followed by CRC
  I2C_queue_receive_data(string("\x12\x34\x37\x56\x78\x7d", 6));

  uint32_t serial = 0;
  ASSERT_TRUE(sht4x_read_serial(&i2c, SHT4X_ADDRESS_C, &serial));
  ASSERT_EQ(0x12345678, serial);

  ASSERT_EQ(I2C_get_transmit_history_entry(0), "\x89");
}

TEST_F(sht4x, present)
{
  I2C_queue_receive_data(string("\x12\x34\x37\x56\x78\x7d", 6));
  ASSERT_TRUE(sht4x_sensor_present(&i2c, SHT4X_ADDRESS_C));

  // Corrupted CRC of the second word - sensor must be treated as not present
  I2C_queue_receive_data(string("\x12\x34\x37\x56\x78\x00", 6));
  ASSERT_FALSE(sht4x_sensor_present(&i2c, SHT4X_ADDRESS_C));
}

TEST_F(sht4x, measurement)
{
  // 0x6666 -> 25.00C, 0x8000 -> 56.50% -> 57%
  I2C_queue_receive_data(string("\x66\x66\x93\x80\x00\xa2", 6));

  int32_t temp = 0;
  uint32_t hum = 0;
  ASSERT_TRUE(sht4x_one_shot_measurement(&i2c, SHT4X_ADDRESS_C, SHT4X_PRECISION_HIGH, &temp, &hum));
  ASSERT_EQ(2500, temp);
  ASSERT_EQ(57, hum);

  ASSERT_EQ(I2C_get_transmit_history_entry(0), "\xfd");
}

TEST_F(sht4x, humidity_rounding)
{
  // 0x7BE4 -> 54.494% -> 54%, 0x7BE8 -> 54.502% -> 55% (truncating gave 54 for both)
  I2C_queue_receive_data(string("\x66\x66\x93\x7b\xe4\xa5", 6));
  I2C_queue_receive_data(string("\x66\x66\x93\x7b\xe8\xd8", 6));

  uint32_t hum = 0;
  ASSERT_TRUE(sht4x_one_shot_measurement(&i2c, SHT4X_ADDRESS_C, SHT4X_PRECISION_HIGH, NULL, &hum));
  ASSERT_EQ(54, hum);
  ASSERT_TRUE(sht4x_one_shot_measurement(&i2c, SHT4X_ADDRESS_C, SHT4X_PRECISION_HIGH, NULL, &hum));
  ASSERT_EQ(55, hum);
}

TEST_F(sht4x, negative_temperature)
{
  // 0x1000 -> -34.07C
  I2C_queue_receive_data(string("\x10\x00\xef\x80\x00\xa2", 6));

  int32_t temp = 0;
  ASSERT_TRUE(sht4x_one_shot_measurement(&i2c, SHT4X_ADDRESS_C, SHT4X_PRECISION_HIGH, &temp, NULL));
  ASSERT_EQ(-3407, temp);
}

TEST_F(sht4x, humidity_clipping)
{
  // Unlike SHT3x, raw humidity maps to -6..119%, so both ends must be clipped
  I2C_queue_receive_data(string("\x66\x66\x93\x00\x00\x81", 6));
  I2C_queue_receive_data(string("\x66\x66\x93\xff\xff\xac", 6));

  uint32_t hum = 0xff;
  ASSERT_TRUE(sht4x_one_shot_measurement(&i2c, SHT4X_ADDRESS_C, SHT4X_PRECISION_HIGH, NULL, &hum));
  ASSERT_EQ(0, hum);

  ASSERT_TRUE(sht4x_one_shot_measurement(&i2c, SHT4X_ADDRESS_C, SHT4X_PRECISION_HIGH, NULL, &hum));
  ASSERT_EQ(100, hum);
}

TEST_F(sht4x, precision)
{
  I2C_queue_receive_data(string("\x66\x66\x93\x80\x00\xa2", 6));
  I2C_queue_receive_data(string("\x66\x66\x93\x80\x00\xa2", 6));
  I2C_queue_receive_data(string("\x66\x66\x93\x80\x00\xa2", 6));

  ASSERT_TRUE(sht4x_one_shot_measurement(&i2c, SHT4X_ADDRESS_C, SHT4X_PRECISION_HIGH, NULL, NULL));
  ASSERT_TRUE(sht4x_one_shot_measurement(&i2c, SHT4X_ADDRESS_C, SHT4X_PRECISION_MEDIUM, NULL, NULL));
  ASSERT_TRUE(sht4x_one_shot_measurement(&i2c, SHT4X_ADDRESS_C, SHT4X_PRECISION_LOW, NULL, NULL));

  ASSERT_EQ(I2C_get_transmit_history_entry(0), "\xfd");
  ASSERT_EQ(I2C_get_transmit_history_entry(1), "\xf6");
  ASSERT_EQ(I2C_get_transmit_history_entry(2), "\xe0");
}

TEST_F(sht4x, crc_error)
{
  // Corrupted CRC of the temperature word
  I2C_queue_receive_data(string("\x66\x66\x00\x80\x00\xa2", 6));

  int32_t temp = 0;
  uint32_t hum = 0;
  ASSERT_FALSE(sht4x_one_shot_measurement(&i2c, SHT4X_ADDRESS_C, SHT4X_PRECISION_HIGH, &temp, &hum));
}
