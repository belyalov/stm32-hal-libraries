// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#ifndef __DEBUG_H
#define __DEBUG_H

#include "main.h"

#ifndef DEBUG_UART_TIMEOUT
#define DEBUG_UART_TIMEOUT 100
#endif

#ifdef __cplusplus
#define EXPORT extern "C"
#else
#define EXPORT
#endif

EXPORT void debug_print_str(UART_HandleTypeDef *uart, const char *msg);
EXPORT void debug_print_strln(UART_HandleTypeDef *uart, const char *msg);
EXPORT void debug_print_uint64(UART_HandleTypeDef *uart, const char *msg, uint64_t val);
EXPORT void debug_print_uint64ln(UART_HandleTypeDef *uart, const char *msg, uint64_t val);
EXPORT void debug_print_int64(UART_HandleTypeDef *uart, const char *msg, int64_t val);
EXPORT void debug_print_int64ln(UART_HandleTypeDef *uart, const char *msg, int64_t val);
EXPORT void debug_print_hex64(UART_HandleTypeDef *uart, const char *msg, uint64_t val);
EXPORT void debug_print_hex64ln(UART_HandleTypeDef *uart, const char *msg, uint64_t val);

// Converts (un)signed int64 into string.
// Returns pointer to first symbol within this buffer.
EXPORT uint8_t *debug_uint64_to_string(uint64_t value, uint8_t *buf, uint8_t len);
EXPORT uint8_t *debug_int64_to_string(int64_t value, uint8_t *buf, uint8_t len);
EXPORT uint8_t *debug_uint64_to_hexstring(uint64_t value, uint8_t *buf, uint8_t len);


#endif
