// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#include <assert.h>
#include <deque>
#include <string>
#include <string.h>

#include "main.h"
#include "test_mocks.h"

using namespace std;

static deque<string> spi_transmit_history;
static deque<string> spi_transmit_queue;

static deque<string> i2c_transmit_history;
static deque<string> i2c_transmit_queue;

static deque<string> uart_transmit_history;


// SPI mock interface: check history / schedule data to be received
string SPI_get_transmit_history_entry(size_t index)
{
  return spi_transmit_history[index];
}

void SPI_queue_receive_data(const string& data)
{
  spi_transmit_queue.push_back(data);
}

void SPI_clear_transmit_history()
{
  spi_transmit_history.clear();
}

void SPI_clear_transmit_queue()
{
  spi_transmit_queue.clear();
}

// SPI mocks
EXPORT HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
  spi_transmit_history.push_back(string((const char*)pData, Size));

  return HAL_OK;
}

EXPORT HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
  assert(spi_transmit_queue.size() > 0);

  string& data = spi_transmit_queue.front();
  if (data.size() != Size) {
    printf("SPI wants %d bytes, but %lu scheduled\n", Size, data.size());
    assert(data.size() == Size);
  }

  memcpy(pData, data.data(), Size);
  spi_transmit_queue.pop_front();

  return HAL_OK;
}

EXPORT HAL_StatusTypeDef HAL_SPI_Receive_DMA(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size)
{
  return HAL_SPI_Receive(hspi, pData, Size, 0);
}

EXPORT HAL_StatusTypeDef HAL_SPI_Transmit_DMA(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size)
{
  return HAL_SPI_Transmit(hspi, pData, Size, 0);
}

// I2C mock interface: check history / schedule data to be received
string I2C_get_transmit_history_entry(size_t index)
{
  return i2c_transmit_history[index];
}

void I2C_queue_receive_data(const string& data)
{
  i2c_transmit_queue.push_back(data);
}

void I2C_clear_transmit_history()
{
  i2c_transmit_history.clear();
}

void I2C_clear_transmit_queue()
{
  i2c_transmit_queue.clear();
}


// UART history
string UART_get_transmit_history_entry(size_t index)
{
  return uart_transmit_history[index];
}

void UART_clear_transmit_history()
{
  uart_transmit_history.clear();
}

size_t UART_get_transmit_history_size()
{
  return uart_transmit_history.size();
}

// I2C mocks

EXPORT HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
  i2c_transmit_history.push_back(string((const char*)pData, Size));

  return HAL_OK;
}

EXPORT HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
  assert(i2c_transmit_queue.size() > 0);

  string& data = i2c_transmit_queue.front();
  if (data.size() != Size) {
    printf("I2C wants %d bytes, but %lu scheduled\n", Size, data.size());
    assert(data.size() == Size);
  }

  memcpy(pData, data.data(), Size);
  i2c_transmit_queue.pop_front();

  return HAL_OK;
}

EXPORT HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    return HAL_I2C_Master_Receive(hi2c, DevAddress, pData, Size, Timeout);
}


// UART mocks
EXPORT HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart2, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
  uart_transmit_history.push_back(string((const char*)pData, Size));

  return HAL_OK;
}

// GPIO mocks
EXPORT void HAL_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState)
{
}

// Misc
EXPORT void HAL_Delay(uint32_t Delay)
{
}
