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

// Convenient macros to use DEBUG_UART to send debug messages to
#ifdef DEBUG_UART
extern UART_HandleTypeDef DEBUG_UART;
#ifdef DEBUG_SEM
#include "cmsis_os.h"
extern osSemaphoreId DEBUG_SEM;
#define DEBUG_UART_LOCK    osSemaphoreWait(DEBUG_SEM, osWaitForever)
#define DEBUG_UART_UNLOCK  osSemaphoreRelease(DEBUG_SEM)
#else
#define DEBUG_UART_LOCK
#define DEBUG_UART_UNLOCK
#endif
#define DEBUG(s)                   DEBUG_UART_LOCK; debug_print_str(&DEBUG_UART, s); DEBUG_UART_UNLOCK;
#define DEBUGLN(s)                 DEBUG_UART_LOCK; debug_print_strln(&DEBUG_UART, s); DEBUG_UART_UNLOCK;
#define DEBUG_STR_LN(s1, s2)       DEBUG_UART_LOCK; debug_print_strstrln(&DEBUG_UART, s1, s2); DEBUG_UART_UNLOCK;
#define DEBUG_UINT(s, v)           DEBUG_UART_LOCK; debug_print_uint64(&DEBUG_UART, s, v); DEBUG_UART_UNLOCK;
#define DEBUG_UINT_LN(s, v)        DEBUG_UART_LOCK; debug_print_uint64ln(&DEBUG_UART, s, v); DEBUG_UART_UNLOCK;
#define DEBUG_INT(s, v)            DEBUG_UART_LOCK; debug_print_int64(&DEBUG_UART, s, v); DEBUG_UART_UNLOCK;
#define DEBUG_INT_LN(s, v)         DEBUG_UART_LOCK; debug_print_int64ln(&DEBUG_UART, s, v); DEBUG_UART_UNLOCK;
#define DEBUG_UINT_HEX(s, v)       DEBUG_UART_LOCK; debug_print_hex64(&DEBUG_UART, s, v); DEBUG_UART_UNLOCK;
#define DEBUG_UINT_HEXLN(s, v)     DEBUG_UART_LOCK; debug_print_hex64ln(&DEBUG_UART, s, v); DEBUG_UART_UNLOCK;
#endif

EXPORT void debug_print_str(UART_HandleTypeDef *uart, const char *msg);
EXPORT void debug_print_strln(UART_HandleTypeDef *uart, const char *msg);
EXPORT void debug_print_strstrln(UART_HandleTypeDef *uart, const char *msg1, const char *msg2);
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
