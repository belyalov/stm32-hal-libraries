// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.
#include "debug.h"
#include <stdio.h>
#include <string.h>

uint8_t _stm32_hal_debug_buffer[DEBUG_BUFFER_SIZE];

static uint8_t *uint_to_str(uint64_t value, uint8_t base, uint8_t *buf, uint8_t len)
{
  uint8_t *current = buf + len - 1;

  if (len < 2 || buf == NULL) {
    return buf;
  }

  // Null terminated string
  *current = 0;

  // Special case for zero value
  if (value == 0) {
    current--;
    *current = '0';
    return current;
  }

  while (value > 0 && current != buf) {
    uint8_t mod = value % base;
    current--;
    *current = mod > 9 ? 'W' + mod : '0' + mod;
    value /= base;
  }

  return current;
}

uint8_t *debug_uint64_to_hexstring(uint64_t value, uint8_t *buf, uint8_t len)
{
  return uint_to_str(value, 16, buf, len);
}

uint8_t *debug_uint64_to_string(uint64_t value, uint8_t *buf, uint8_t len)
{
  return uint_to_str(value, 10, buf, len);
}

uint8_t *debug_int64_to_string(int64_t value, uint8_t *buf, uint8_t len)
{
  if (value >= 0) {
    return debug_uint64_to_string(value, buf, len);
  }

  if (len < 3) {
    return buf;
  }

  // Reserve space for sign
  buf++;
  uint8_t *res = uint_to_str(0 - value, 10, buf, len - 1);
  res--;
  *res = '-';

  return res;
}

void debug_print_str(UART_HandleTypeDef *uart, const char *msg)
{
    HAL_UART_Transmit(uart, (uint8_t*)msg, strlen(msg), DEBUG_UART_TIMEOUT);
}

void debug_print_strln(UART_HandleTypeDef *uart, const char *msg)
{
    HAL_UART_Transmit(uart, (uint8_t*)msg, strlen(msg), DEBUG_UART_TIMEOUT);
    HAL_UART_Transmit(uart, (uint8_t*)"\r\n", 2, DEBUG_UART_TIMEOUT);
}

void debug_print_uint64(UART_HandleTypeDef *uart, const char *msg, uint64_t val)
{
    uint8_t *sval = debug_uint64_to_string(val, _stm32_hal_debug_buffer, DEBUG_BUFFER_SIZE);
    HAL_UART_Transmit(uart, (uint8_t*)msg, strlen(msg), DEBUG_UART_TIMEOUT);
    HAL_UART_Transmit(uart, sval, strlen((char*)sval), DEBUG_UART_TIMEOUT);
}

void debug_print_uint64ln(UART_HandleTypeDef *uart, const char *msg, uint64_t val)
{
    uint8_t *sval = debug_uint64_to_string(val, _stm32_hal_debug_buffer, DEBUG_BUFFER_SIZE);
    HAL_UART_Transmit(uart, (uint8_t*)msg, strlen(msg), DEBUG_UART_TIMEOUT);
    HAL_UART_Transmit(uart, sval, strlen((char*)sval), DEBUG_UART_TIMEOUT);
    HAL_UART_Transmit(uart, (uint8_t*)"\r\n", 2, DEBUG_UART_TIMEOUT);
}

void debug_print_int64(UART_HandleTypeDef *uart, const char *msg, int64_t val)
{
    uint8_t *sval = debug_int64_to_string(val, _stm32_hal_debug_buffer, DEBUG_BUFFER_SIZE);
    HAL_UART_Transmit(uart, (uint8_t*)msg, strlen(msg), DEBUG_UART_TIMEOUT);
    HAL_UART_Transmit(uart, sval, strlen((char*)sval), DEBUG_UART_TIMEOUT);
}

void debug_print_int64ln(UART_HandleTypeDef *uart, const char *msg, int64_t val)
{
    uint8_t *sval = debug_int64_to_string(val, _stm32_hal_debug_buffer, DEBUG_BUFFER_SIZE);
    HAL_UART_Transmit(uart, (uint8_t*)msg, strlen(msg), DEBUG_UART_TIMEOUT);
    HAL_UART_Transmit(uart, sval, strlen((char*)sval), DEBUG_UART_TIMEOUT);
    HAL_UART_Transmit(uart, (uint8_t*)"\r\n", 2, DEBUG_UART_TIMEOUT);
}

void debug_print_hex64(UART_HandleTypeDef *uart, const char *msg, uint64_t val)
{
    uint8_t *sval = debug_uint64_to_hexstring(val, _stm32_hal_debug_buffer, DEBUG_BUFFER_SIZE);
    HAL_UART_Transmit(uart, (uint8_t*)msg, strlen(msg), DEBUG_UART_TIMEOUT);
    HAL_UART_Transmit(uart, sval, strlen((char*)sval), DEBUG_UART_TIMEOUT);
}

void debug_print_hex64ln(UART_HandleTypeDef *uart, const char *msg, uint64_t val)
{
    uint8_t *sval = debug_uint64_to_hexstring(val, _stm32_hal_debug_buffer, DEBUG_BUFFER_SIZE);
    HAL_UART_Transmit(uart, (uint8_t*)msg, strlen(msg), DEBUG_UART_TIMEOUT);
    HAL_UART_Transmit(uart, sval, strlen((char*)sval), DEBUG_UART_TIMEOUT);
    HAL_UART_Transmit(uart, (uint8_t*)"\r\n", 2, DEBUG_UART_TIMEOUT);
}

void debug_print_strstrln(UART_HandleTypeDef *uart, const char *msg1, const char *msg2)
{
    HAL_UART_Transmit(uart, (uint8_t*)msg1, strlen(msg1), DEBUG_UART_TIMEOUT);
    HAL_UART_Transmit(uart, (uint8_t*)msg2, strlen(msg2), DEBUG_UART_TIMEOUT);
    HAL_UART_Transmit(uart, (uint8_t*)"\r\n", 2, DEBUG_UART_TIMEOUT);
}
