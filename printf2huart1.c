#include <errno.h>
#include <sys/unistd.h>
#include <string.h>
#include "main.h"

#ifndef UART_TIMEOUT
#define UART_TIMEOUT    1000
#endif

#ifndef UART_DMA_BUFFER_SIZE
#define UART_DMA_BUFFER_SIZE 128
#endif

extern UART_HandleTypeDef huart1;

static uint8_t _printf_uart_dma_buffer[UART_DMA_BUFFER_SIZE];

// These functions are implemented in the GCC C library as
// stub routines with "weak" linkage so just re-define it
// to write to UART1
int _write(int file, char *data, int len)
{
   if ((file != STDOUT_FILENO) && (file != STDERR_FILENO)) {
      errno = EBADF;
      return -1;
   }

   HAL_StatusTypeDef res = HAL_OK;

   // If TX DMA enabled for uart1, use it
   if (huart1.hdmatx != NULL) {
      memcpy(_printf_uart_dma_buffer, data, len);
      HAL_UART_Transmit_DMA(&huart1, _printf_uart_dma_buffer, len);
   } else {
      res = HAL_UART_Transmit(&huart1, (uint8_t*)data, len, UART_TIMEOUT);
   }

   return (res == HAL_OK ? len : 0);
}
