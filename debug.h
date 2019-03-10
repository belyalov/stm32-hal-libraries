// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#ifndef __DEBUG_H
#define __DEBUG_H

#include "main.h"

#ifdef DEBUG_PRINT

#ifndef DEBUG_UART_TIMEOUT
#define DEBUG_UART_TIMEOUT 100
#endif

#define debug_print_str(uart, msg, val)                                              \
  do {                                                                               \
    HAL_UART_Transmit(uart, (uint8_t*)msg, strlen(msg), DEBUG_UART_TIMEOUT);         \
    uint16_t _s_val_len = strlen((char*)val);                                        \
    if (_s_val_len)                                                                  \
      HAL_UART_Transmit(uart, (uint8_t*)val, strlen((char*)val), DEBUG_UART_TIMEOUT);\
  } while (0);

#define debug_print_strln(uart, msg, val)                                            \
  do {                                                                               \
    HAL_UART_Transmit(uart, (uint8_t*)msg, strlen(msg), DEBUG_UART_TIMEOUT);         \
    uint16_t _s_val_len = strlen((char*)val);                                        \
    if (_s_val_len)                                                                  \
      HAL_UART_Transmit(uart, (uint8_t*)val, strlen((char*)val), DEBUG_UART_TIMEOUT);\
    HAL_UART_Transmit(uart, (uint8_t*)"\r\n", 2, DEBUG_UART_TIMEOUT);                \
  } while (0);

#define debug_print_uint64(uart, msg, val)                                        \
  do {                                                                            \
    uint8_t buf[16];                                                              \
    uint8_t *sval = debug_uint64_to_string(val, buf, sizeof(buf));                \
    HAL_UART_Transmit(uart, (uint8_t*)msg, strlen(msg), DEBUG_UART_TIMEOUT);      \
    HAL_UART_Transmit(uart, sval, strlen((char*)sval), DEBUG_UART_TIMEOUT);       \
  } while (0);

#define debug_print_uint64ln(uart, msg, val)                                      \
  do {                                                                            \
    uint8_t buf[16];                                                              \
    uint8_t *sval = debug_uint64_to_string(val, buf, sizeof(buf));                \
    HAL_UART_Transmit(uart, (uint8_t*)msg, strlen(msg), DEBUG_UART_TIMEOUT);      \
    HAL_UART_Transmit(uart, sval, strlen((char*)sval), DEBUG_UART_TIMEOUT);       \
    HAL_UART_Transmit(uart, (uint8_t*)"\r\n", 2, DEBUG_UART_TIMEOUT);             \
  } while (0);

#define debug_print_int64(uart, msg, val)                                        \
  do {                                                                           \
    uint8_t buf[16];                                                             \
    uint8_t *sval = debug_int64_to_string(val, buf, sizeof(buf));                \
    HAL_UART_Transmit(uart, (uint8_t*)msg, strlen(msg), DEBUG_UART_TIMEOUT);     \
    HAL_UART_Transmit(uart, sval, strlen((char*)sval), DEBUG_UART_TIMEOUT);      \
  } while (0);

#define debug_print_int64ln(uart, msg, val)                                      \
  do {                                                                           \
    uint8_t buf[16];                                                             \
    uint8_t *sval = debug_int64_to_string(val, buf, sizeof(buf));                \
    HAL_UART_Transmit(uart, (uint8_t*)msg, strlen(msg), DEBUG_UART_TIMEOUT);     \
    HAL_UART_Transmit(uart, sval, strlen((char*)sval), DEBUG_UART_TIMEOUT);      \
    HAL_UART_Transmit(uart, (uint8_t*)"\r\n", 2, DEBUG_UART_TIMEOUT);            \
  } while (0);

#define debug_print_hex64(uart, msg, val)                                        \
  do {                                                                           \
    uint8_t buf[16];                                                             \
    uint8_t *sval = debug_uint64_to_hexstring(val, buf, sizeof(buf));            \
    HAL_UART_Transmit(uart, (uint8_t*)msg, strlen(msg), DEBUG_UART_TIMEOUT);     \
    HAL_UART_Transmit(uart, sval, strlen((char*)sval), DEBUG_UART_TIMEOUT);      \
  } while (0);

#define debug_print_hex64ln(uart, msg, val)                                      \
  do {                                                                           \
    uint8_t buf[16];                                                             \
    uint8_t *sval = debug_uint64_to_hexstring(val, buf, sizeof(buf));            \
    HAL_UART_Transmit(uart, (uint8_t*)msg, strlen(msg), DEBUG_UART_TIMEOUT);     \
    HAL_UART_Transmit(uart, sval, strlen((char*)sval), DEBUG_UART_TIMEOUT);      \
    HAL_UART_Transmit(uart, (uint8_t*)"\r\n", 2, DEBUG_UART_TIMEOUT);            \
  } while (0);

#else // ifdef DEBUG

#define debug_print_str(uart, msg, val)
#define debug_print_strln(uart, msg, val)
#define debug_print_uint64(uart, msg, val)
#define debug_print_uint64ln(uart, msg, val)
#define debug_print_int64(uart, msg, val)
#define debug_print_int64ln(uart, msg, val)
#define debug_print_hex64(uart, msg, val)
#define debug_print_hex64ln(uart, msg, val)

#endif // ifdef DEBUG

#ifdef __cplusplus
#define EXPORT extern "C"
#else
#define EXPORT
#endif

// Converts (un)signed int64 into string.
// Returns pointer to first symbol within this buffer.
EXPORT uint8_t *debug_uint64_to_string(uint64_t value, uint8_t *buf, uint8_t len);
EXPORT uint8_t *debug_int64_to_string(int64_t value, uint8_t *buf, uint8_t len);
EXPORT uint8_t *debug_uint64_to_hexstring(uint64_t value, uint8_t *buf, uint8_t len);


#endif
