// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#include <gtest/gtest.h>

#define DEBUG_PRINT

#include "debug.h"
#include "test_mocks.h"

using namespace std;

UART_HandleTypeDef *uart = NULL;

TEST(debug, uint_to_string)
{
  map<uint64_t, string> runs = {
    {0, "0"},
    {1, "1"},
    {10, "10"},
    {11, "11"},
    {255, "255"},
    {123456, "123456"},
    {9123456, "9123456"},
    {3232235831, "3232235831"},
    {4294967295, "4294967295"},
    {94294967295, "94294967295"},
    {194294967295, "94294967295"},  // buffer overflow
  };

  // Buffer for conversion
  uint8_t buf[12] = {};

  for (auto it = runs.begin(); it != runs.end(); ++it) {
    uint8_t *res = debug_uint64_to_string(it->first, buf, sizeof(buf));
    // Check len
    uint8_t len = buf + sizeof(buf) - res;
    ASSERT_EQ(len, it->second.size() + 1);
    // Check result
    ASSERT_EQ(it->second, string((char*)res));
  }
}

TEST(debug, uint_to_hex)
{
  map<uint64_t, string> runs = {
    {0, "0"},
    {1, "1"},
    {9, "9"},
    {0xa, "a"},
    {0xb, "b"},
    {0xf, "f"},
    {0xff, "ff"},
    {0xfafa, "fafa"},
    {0xaaff, "aaff"},
    {0xaabbccdd, "aabbccdd"},
    {0xaabbccddeeff, "aabbccddeeff"},
    {0x3138470531383135, "3138470531383135"},
  };

  // Buffer for conversion
  uint8_t buf[17] = {};

  for (auto it = runs.begin(); it != runs.end(); ++it) {
    uint8_t *res = debug_uint64_to_hexstring(it->first, buf, sizeof(buf));
    // Check len
    uint8_t len = buf + sizeof(buf) - res;
    ASSERT_EQ(len, it->second.size() + 1);
    // Check result
    ASSERT_EQ(it->second, string((char*)res));
  }
}

TEST(debug, int_to_string)
{
  map<uint64_t, string> runs = {
    {0, "0"},
    {1, "1"},
    {10, "10"},
    {255, "255"},
    {9123456, "9123456"},
    {-1, "-1"},
    {-99, "-99"},
    {-127, "-127"},
    {-4294967295, "-4294967295"},
    {-14294967295, "-4294967295"}, // buffer overflow
  };

  // Buffer for conversion
  uint8_t buf[12] = {};

  for (auto it = runs.begin(); it != runs.end(); ++it) {
    uint8_t *res = debug_int64_to_string(it->first, buf, sizeof(buf));
    // Check len
    uint8_t len = buf + sizeof(buf) - res;
    ASSERT_EQ(len, it->second.size() + 1);
    // Check result
    ASSERT_EQ(it->second, string((char*)res));
  }
}

TEST(debug, debug_print_uint64)
{
  map<pair<string, uint64_t>, string> runs = {
    { {"value", 0}, "value0"},
    { {"value ", 100}, "value 100"},
  };

  for (auto it = runs.begin(); it != runs.end(); ++it) {
    UART_clear_transmit_history();
    // test macro
    debug_print_uint64(uart, it->first.first.c_str(), it->first.second);
    debug_print_uint64ln(uart, it->first.first.c_str(), it->first.second);
    // Concat UART history
    string result;
    for (size_t i = 0; i < UART_get_transmit_history_size(); i++) {
      result += UART_get_transmit_history_entry(i);
    }
    // Ensure that value is what we've been expected
    ASSERT_EQ(result, it->second + it->second + "\r\n");
  }
}

TEST(debug, debug_print_int64)
{
  map<pair<string, int64_t>, string> runs = {
    { {"value ~ ", -100}, "value ~ -100"},
    { {"value ~ ", -1}, "value ~ -1"},
    { {"value", 0}, "value0"},
    { {"value=", 1}, "value=1"},
  };

  for (auto it = runs.begin(); it != runs.end(); ++it) {
    UART_clear_transmit_history();
    // test macros
    debug_print_int64(uart, it->first.first.c_str(), it->first.second);
    debug_print_int64ln(uart, it->first.first.c_str(), it->first.second);
    // Concat UART history
    string result;
    for (size_t i = 0; i < UART_get_transmit_history_size(); i++) {
      result += UART_get_transmit_history_entry(i);
    }
    // Ensure that value is what we've been expected
    ASSERT_EQ(result, it->second + it->second + "\r\n");
  }
}

TEST(debug, debug_print_hex64)
{
  map<pair<string, int64_t>, string> runs = {
    { {"value 0x", 0xff}, "value 0xff"},
    { {"value fafa", 0xfafa}, "value fafafafa"},
  };

  for (auto it = runs.begin(); it != runs.end(); ++it) {
    UART_clear_transmit_history();
    // test macro
    debug_print_hex64(uart, it->first.first.c_str(), it->first.second);
    debug_print_hex64ln(uart, it->first.first.c_str(), it->first.second);
    // Concat UART history
    string result;
    for (size_t i = 0; i < UART_get_transmit_history_size(); i++) {
      result += UART_get_transmit_history_entry(i);
    }
    // Ensure that value is what we've been expected
    ASSERT_EQ(result, it->second + it->second + "\r\n");
  }
}

TEST(debug, debug_print_str)
{
  map<string, string> runs = {
    {"value", "value"},
    {"value 2", "value 2"},
  };

  for (auto it = runs.begin(); it != runs.end(); ++it) {
    UART_clear_transmit_history();
    // test macro
    debug_print_str(uart, it->first.c_str());
    debug_print_strln(uart, it->first.c_str());
    // Concat UART history
    string result;
    for (size_t i = 0; i < UART_get_transmit_history_size(); i++) {
      result += UART_get_transmit_history_entry(i);
    }
    // Ensure that value is what we've been expected
    ASSERT_EQ(result, it->second + it->second + "\r\n");
  }
}
