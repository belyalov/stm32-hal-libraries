// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#include <gtest/gtest.h>

#include "si7021.h"
#include "test_mocks.h"

using namespace std;


class si7021 : public ::testing::Test {
protected:
  void SetUp() override {
    I2C_clear_transmit_history();
    I2C_clear_transmit_queue();
  }

  I2C_HandleTypeDef i2c;
};

TEST_F(si7021, id)
{
  // Setup I2C data to be received by si7021
  I2C_queue_receive_data("\x11\x22\x33\x44");
  I2C_queue_receive_data("\x55\x66\x77\x88");

  // Run test
  auto res = si7021_read_id(&i2c);
  ASSERT_EQ(0x1122334455667788, res);
}

TEST_F(si7021, temperature)
{
  // Setup I2C data to be received by si7021
  // 0x66 0x9c -> 23.58 temp
  // Third byte is CRC which is not used yet
  I2C_queue_receive_data(string("\x66\x9c\x00", 3));
  I2C_queue_receive_data(string("\x66\x9c\x00", 3));

  // Run test
  auto res = si7021_measure_temperature(&i2c);
  ASSERT_EQ(2358, res);

  // One more variation - read temperature that has been measured by
  // measure humidity call
  res = si7021_read_previous_temperature(&i2c);
  ASSERT_EQ(2358, res);

  // Ensure that library sent correct data to i2c
  ASSERT_EQ(I2C_get_transmit_history_entry(0), "\xf3");
  ASSERT_EQ(I2C_get_transmit_history_entry(1), "\xe0");
}

TEST_F(si7021, humidity)
{
  // Setup I2C data to be received by si7021
  // 0x6c 0xb6 -> 47 humidity
  // Third byte is CRC which is not used yet
  I2C_queue_receive_data(string("\x6c\xb6\x00", 3));

  // Run test
  auto res = si7021_measure_humidity(&i2c);
  ASSERT_EQ(47, res);

  // Ensure that library sent correct data to i2c
  ASSERT_EQ(I2C_get_transmit_history_entry(0), "\xf5");
}

