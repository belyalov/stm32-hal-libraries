// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.
#include <string.h>
#include "main.h"
#include "usart.h"
#include "wiz5500.h"
#include "htons.h"
#include "debug.h"


#define DEBUG(s)                 debug_print_str(&huart1, (s))
#define DEBUGLN(s)               debug_print_strln(&huart1, (s))
#define DEBUG_UINT(s, v)         debug_print_uint64(&huart1, (s), v)
#define DEBUG_UINT_LN(s, v)      debug_print_uint64ln(&huart1, (s), v)
#define DEBUG_UINT_HEX(s, v)     debug_print_hex64(&huart1, (s), v)
#define DEBUG_UINT_HEXLN(s, v)   debug_print_hex64ln(&huart1, (s), v)


// First level Registers (shift left by 3 before use) //
#define REGISTER_COMMON          0x00
#define REGISTER_SOCKET_N(n)     (n * 4 + 1)

#define MAX_SOCKET_NUMBER        7

// Add this value to REGISTER_SOCKET_N before left shitf //
#define REGISTER_SOCKET_TX       0x1
#define REGISTER_SOCKET_RX       0x2

// Common register addresses //
#define COMMON_MODE                 0x0000
#define COMMON_MODE_PPPOE           (1 << 3)
#define COMMON_MODE_PING            (1 << 4)
#define COMMON_MODE_WOL             (1 << 5)
#define COMMON_PHYCFGR              0x002E
#define COMMON_PHYCFGR_OPMD_SW      (1 << 6)
#define COMMON_PHYCFGR_DPX          (1 << 2)
#define COMMON_PHYCFGR_SPD          (1 << 1)
#define COMMON_PHYCFGR_LNK          (1)
#define COMMON_SOURCE_MAC           0x0009
#define COMMON_GATEWAY_IP           0x0001
#define COMMON_SUBNET_MASK          0x0005
#define COMMON_SOURCE_IP            0x000F
#define COMMON_INTERRUPT            0x0015
#define COMMON_SOCKET_INTERRUPT     0x0017
#define COMMON_SOCKET_INTERRUPT_MASK 0x0018
#define COMMON_CHIP_VERSION         0x0039

// Socket registers addresses //
#define SOCKET_MODE                 0x0000
#define SOCKET_COMMAND              0x0001
#define SOCKET_INTERRUPT            0x0002
#define SOCKET_INTERRUPT_MASK       0x002C
#define SOCKET_STATUS               0x0003
#define SOCKET_SOURCE_PORT          0x0004
#define SOCKET_DESTINATION_MAC      0x0006
#define SOCKET_DESTINATION_IP       0x000C
#define SOCKET_DESTINATION_PORT     0x0010
#define SOCKET_MSS                  0x0012
#define SOCKET_TOS                  0x0015
#define SOCKET_TTL                  0x0016
#define SOCKET_FRAGMENT             0x002D
#define SOCKET_KEEPALIVE_TIMEOUT    0x002F

#define SOCKET_RX_BUFFER_SIZE       0x001E
#define SOCKET_RX_RECEIVED_SIZE     0x0026
#define SOCKET_RX_READ_POINTER      0x0028
#define SOCKET_RX_WRITE_POINTER     0x002A

#define SOCKET_TX_BUFFER_SIZE       0x001F
#define SOCKET_TX_FREE_SIZE         0x0020
#define SOCKET_TX_READ_POINTER      0x0022
#define SOCKET_TX_WRITE_POINTER     0x0024

// Socket commands (for register SOCKET_COMMAND)
#define SOCKET_COMMAND_OPEN         0x01
#define SOCKET_COMMAND_LISTEB       0x02
#define SOCKET_COMMAND_CONNECT      0x04
#define SOCKET_COMMAND_DISCONNECT   0x08
#define SOCKET_COMMAND_CLOSE        0x10
#define SOCKET_COMMAND_SEND         0x20
#define SOCKET_COMMAND_SEND_MAC     0x21
#define SOCKET_COMMAND_SEND_KEEP    0x22
#define SOCKET_COMMAND_RECV         0x40

// Data transfer mode
#define DATA_READ                   (0x0 << 2)
#define DATA_WRITE                  (0x1 << 2)
#define DATA_MODE_VARIABLE          0x0
#define DATA_MODE_FIXED_1           0x01
#define DATA_MODE_FIXED_2           0x02
#define DATA_MODE_FIXED_4           0x03


#define DEFAULT_SPI_TIMEOUT         1000 // ms


static uint8_t _read_register8(wiz5500 *wiz, uint16_t addr, uint8_t reg)
{
  // Address phase (16 bit) + Control phase (register + mode)
  uint8_t payload[3] = {addr >> 8, addr & 0xff, (reg << 3) | DATA_MODE_FIXED_1 | DATA_READ};
  uint8_t value = 0;

  // Start SPI transaction
  HAL_GPIO_WritePin(wiz->nss_port, wiz->nss_pin, GPIO_PIN_RESET);
  uint32_t res1 = HAL_SPI_Transmit(wiz->spi, (uint8_t*)payload, sizeof(payload), wiz->spi_timeout);
  uint32_t res2 = HAL_SPI_Receive(wiz->spi, &value, 1, wiz->spi_timeout);
  // End SPI transaction
  HAL_GPIO_WritePin(wiz->nss_port, wiz->nss_pin, GPIO_PIN_SET);

  if (res1 != HAL_OK || res2 != HAL_OK) {
    DEBUGLN("transmit / receive failed");
  }

  return value;
}

static uint8_t _write_register8(wiz5500 *wiz, uint16_t addr, uint8_t reg, uint8_t value)
{
  // Address phase (16 bit) + Control phase (register + mode) + value
  uint8_t payload[4] = {addr >> 8, addr & 0xff, (reg << 3) | DATA_MODE_FIXED_1 | DATA_WRITE, value};
  // Start SPI transaction
  HAL_GPIO_WritePin(wiz->nss_port, wiz->nss_pin, GPIO_PIN_RESET);
  uint32_t res = HAL_SPI_Transmit(wiz->spi, (uint8_t*)payload, sizeof(payload), wiz->spi_timeout);
  // End SPI transaction
  HAL_GPIO_WritePin(wiz->nss_port, wiz->nss_pin, GPIO_PIN_SET);

  if (res != HAL_OK) {
    DEBUGLN("transmit / receive failed");
  }

  return res;
}

static uint16_t _read_register16(wiz5500 *wiz, uint16_t addr, uint8_t reg)
{
  // Address phase (16 bit) + Control phase (register + mode) + value
  uint8_t payload[3] = {addr >> 8, addr & 0xff, (reg << 3) | DATA_MODE_FIXED_2 | DATA_READ};

  // Start SPI transaction
  HAL_GPIO_WritePin(wiz->nss_port, wiz->nss_pin, GPIO_PIN_RESET);
  uint32_t res1 = HAL_SPI_Transmit(wiz->spi, (uint8_t*)payload, sizeof(payload), wiz->spi_timeout);
  uint32_t res2 = HAL_SPI_Receive(wiz->spi, payload, 2, wiz->spi_timeout);
  // End SPI transaction
  HAL_GPIO_WritePin(wiz->nss_port, wiz->nss_pin, GPIO_PIN_SET);

  if (res1 != HAL_OK || res2 != HAL_OK) {
    DEBUGLN("transmit / receive failed");
  }

  return payload[0] << 8 | payload[1];
}

// Read uint16_t multiple times as per datasheet :)
static uint16_t _read_register16_for_sure(wiz5500 *wiz, uint16_t addr, uint8_t reg)
{
  uint16_t val;

  do {
    val = _read_register16(wiz, addr, reg);
  } while (val != _read_register16(wiz, addr, reg));

  return val;
}

static uint8_t _write_register16(wiz5500 *wiz, uint16_t addr, uint8_t reg, uint16_t value)
{
  // Address phase (16 bit) + Control phase (register + mode) + value
  uint8_t payload[5] = {addr >> 8, addr & 0xff, (reg << 3) | DATA_MODE_FIXED_2 | DATA_WRITE,
                        value >> 8, value &0xff,
  };

  // Start SPI transaction
  HAL_GPIO_WritePin(wiz->nss_port, wiz->nss_pin, GPIO_PIN_RESET);
  uint32_t res = HAL_SPI_Transmit(wiz->spi, (uint8_t*)payload, sizeof(payload), wiz->spi_timeout);
  // End SPI transaction
  HAL_GPIO_WritePin(wiz->nss_port, wiz->nss_pin, GPIO_PIN_SET);

  if (res != HAL_OK) {
    DEBUGLN("transmit / receive failed");
  }

  return res;
}

static uint32_t _read_register32(wiz5500 *wiz, uint16_t addr, uint8_t reg)
{
  // Address phase (16 bit) + Control phase (register + mode)
  uint8_t payload[4] = {addr >> 8, addr & 0xff, (reg << 3) | DATA_MODE_FIXED_4 | DATA_READ};

  // Start SPI transaction
  HAL_GPIO_WritePin(wiz->nss_port, wiz->nss_pin, GPIO_PIN_RESET);
  uint32_t res1 = HAL_SPI_Transmit(wiz->spi, (uint8_t*)payload, 3, wiz->spi_timeout);
  uint32_t res2 = HAL_SPI_Receive(wiz->spi, payload, 4, wiz->spi_timeout);
  // End SPI transaction
  HAL_GPIO_WritePin(wiz->nss_port, wiz->nss_pin, GPIO_PIN_SET);

  if (res1 != HAL_OK || res2 != HAL_OK) {
    DEBUGLN("transmit / receive failed");
  }

  // DEBUG("read: ");
  // for(int i = sizeof(payload) - 1; i >= 0; i--) {
  //   // DEBUG_UINT(" ", payload[i]);
  // }
  // DEBUGLN("");

  return HTONL(*(uint32_t*)payload);
}

static uint8_t _write_register32(wiz5500 *wiz, uint16_t addr, uint8_t reg, uint32_t value)
{
  // Address phase (16 bit) + Control phase (register + mode) + value
  uint8_t payload[7] = {addr >> 8, addr & 0xff, (reg << 3) | DATA_MODE_FIXED_4 | DATA_WRITE};
  uint32_t reversed = HTONL(value);
  memcpy(&payload[3], &reversed, 4);

  // DEBUG("write: ");
  // for(int i =0; i < sizeof(payload); i++) {
  //   DEBUG_UINT(" ", payload[i]);
  // }
  // DEBUGLN("");

  // Start SPI transaction
  HAL_GPIO_WritePin(wiz->nss_port, wiz->nss_pin, GPIO_PIN_RESET);
  uint32_t res = HAL_SPI_Transmit(wiz->spi, (uint8_t*)payload, sizeof(payload), wiz->spi_timeout);
  // End SPI transaction
  HAL_GPIO_WritePin(wiz->nss_port, wiz->nss_pin, GPIO_PIN_SET);

  if (res != HAL_OK) {
    DEBUGLN("transmit / receive failed");
  }

  return res;
}

uint8_t  wiz5500_init(wiz5500 *wiz, SPI_HandleTypeDef *spi, GPIO_TypeDef *nss_port,
                      uint16_t nss_pin)
{
  assert_param(lora && spi);

  // Init params with default values
  wiz->spi = spi;
  wiz->nss_port = nss_port;
  wiz->nss_pin = nss_pin;
  wiz->spi_timeout = DEFAULT_SPI_TIMEOUT;

  return wiz5500_version(wiz) == 0x04 ? WIZ5500_OK : WIZ5500_ERROR;
}

uint32_t wiz5500_version(wiz5500 *wiz)
{
  return _read_register8(wiz, COMMON_CHIP_VERSION, REGISTER_COMMON);
}

uint8_t wiz5500_ping_enable(wiz5500 *wiz)
{
  uint8_t value = _read_register8(wiz, COMMON_MODE, REGISTER_COMMON);
  value &= ~COMMON_MODE_PING;

  return _write_register8(wiz, COMMON_MODE, REGISTER_COMMON, value);
}

uint8_t wiz5500_ping_disable(wiz5500 *wiz)
{
  uint8_t value = _read_register8(wiz, COMMON_MODE, REGISTER_COMMON);
  value |= COMMON_MODE_PING;

  return _write_register8(wiz, COMMON_MODE, REGISTER_COMMON, value);
}

uint8_t wiz5500_link_status(wiz5500 *wiz)
{
  uint8_t value = _read_register8(wiz, COMMON_PHYCFGR, REGISTER_COMMON);

  return value & COMMON_PHYCFGR_LNK ? WIZ5500_LINK_UP : WIZ5500_LINK_DOWN;
}

uint8_t wiz5500_set_link(wiz5500 *wiz, uint8_t mode)
{
  uint8_t value = 0x80 | mode | COMMON_PHYCFGR_OPMD_SW;

  return _write_register8(wiz, COMMON_PHYCFGR, REGISTER_COMMON, value);
}

uint8_t wiz5500_is_link_full_duplex(wiz5500 *wiz)
{
  uint8_t value = _read_register8(wiz, COMMON_PHYCFGR, REGISTER_COMMON);

  return value & COMMON_PHYCFGR_DPX;
}

uint8_t wiz5500_is_link_speed_100mb(wiz5500 *wiz)
{
  uint8_t value = _read_register8(wiz, COMMON_PHYCFGR, REGISTER_COMMON);

  return value & COMMON_PHYCFGR_SPD;
}

uint32_t wiz5500_get_source_ip(wiz5500 *wiz)
{
  return _read_register32(wiz, COMMON_SOURCE_IP, REGISTER_COMMON);
}

uint8_t wiz5500_set_source_ip(wiz5500 *wiz, uint32_t ip)
{
  return _write_register32(wiz, COMMON_SOURCE_IP, REGISTER_COMMON, ip);
}

uint64_t wiz5500_get_mac(wiz5500 *wiz)
{
  // Address phase (16 bit) + Control phase (register + mode)
  uint8_t payload[6] = {
    0,
    COMMON_SOURCE_MAC,
    (REGISTER_COMMON << 3) | DATA_MODE_VARIABLE | DATA_READ,
  };

  // Start SPI transaction
  HAL_GPIO_WritePin(wiz->nss_port, wiz->nss_pin, GPIO_PIN_RESET);
  uint32_t res1 = HAL_SPI_Transmit(wiz->spi, payload, 3, wiz->spi_timeout);
  uint32_t res2 = HAL_SPI_Receive(wiz->spi, payload, 6, wiz->spi_timeout);
  // End SPI transaction
  HAL_GPIO_WritePin(wiz->nss_port, wiz->nss_pin, GPIO_PIN_SET);

  if (res1 != HAL_OK || res2 != HAL_OK) {
    DEBUGLN("transmit / receive failed");
  }

  uint64_t value = 0;
  for(int i = 0; i < sizeof(payload); i++) {
    value |= (uint64_t)payload[i] << (40 - i * 8);
  }

  return value;
}

uint64_t wiz5500_set_mac(wiz5500 *wiz, uint64_t mac)
{
  mac &= 0xffffffffffff; // use only last 6 bytes
  // Address phase (16 bit) + Control phase (register + mode)
  uint8_t payload[9] = {
    0,
    COMMON_SOURCE_MAC,
    (REGISTER_COMMON << 3) | DATA_MODE_VARIABLE | DATA_WRITE,
    mac >> 40,
    mac >> 32,
    mac >> 24,
    mac >> 16,
    mac >> 8,
    mac & 0xff
  };

  // Start SPI transaction
  HAL_GPIO_WritePin(wiz->nss_port, wiz->nss_pin, GPIO_PIN_RESET);
  uint32_t res = HAL_SPI_Transmit(wiz->spi, (uint8_t*)payload, sizeof(payload), wiz->spi_timeout);
  // End SPI transaction
  HAL_GPIO_WritePin(wiz->nss_port, wiz->nss_pin, GPIO_PIN_SET);

  return res;
}

// sockets //

uint8_t wiz5500_set_socket_dst_ip(wiz5500 *wiz, uint8_t socket, uint32_t ip)
{
  return _write_register32(wiz, SOCKET_DESTINATION_IP, REGISTER_SOCKET_N(socket), ip);
}

uint8_t wiz5500_set_socket_dst_port(wiz5500 *wiz, uint8_t socket, uint16_t port)
{
  return _write_register16(wiz, SOCKET_DESTINATION_PORT, REGISTER_SOCKET_N(socket), port);
}

static uint8_t tx_data(wiz5500 *wiz, uint8_t socket, uint8_t *data, uint16_t data_size, uint8_t dma)
{
  // Check free buffer of wiz5500
  uint16_t free_buf = _read_register16_for_sure(wiz, SOCKET_TX_FREE_SIZE, REGISTER_SOCKET_N(socket));
  if (free_buf < data_size) {
    return WIZ5500_TX_FULL;
  }

  // Get TX writer ptr
  uint16_t tx_wr_ptr = _read_register16_for_sure(wiz, SOCKET_TX_WRITE_POINTER, REGISTER_SOCKET_N(socket));

  // Update TX writer ptr: current + data_len
  uint8_t res = _write_register16(wiz, SOCKET_TX_WRITE_POINTER, REGISTER_SOCKET_N(socket), tx_wr_ptr + data_size);
  if (res != HAL_OK) {
    return res;
  }

  // Transfer data
  uint8_t control[3] = {
    tx_wr_ptr >> 8,
    tx_wr_ptr & 0xff,
    ((REGISTER_SOCKET_N(socket) + REGISTER_SOCKET_TX) << 3) | DATA_MODE_VARIABLE | DATA_WRITE,
  };
  HAL_GPIO_WritePin(wiz->nss_port, wiz->nss_pin, GPIO_PIN_RESET);
  res = HAL_SPI_Transmit(wiz->spi, control, sizeof(control), wiz->spi_timeout);
  if (dma) {
    // Do not disable SPI - must be done by following call of *dma_complete()
    return HAL_SPI_Transmit_DMA(wiz->spi, data, data_size);
  } else {
    res = HAL_SPI_Transmit(wiz->spi, data, data_size, wiz->spi_timeout);
    HAL_GPIO_WritePin(wiz->nss_port, wiz->nss_pin, GPIO_PIN_SET);
  }

  return res;
}

static uint8_t rx_data_offset(wiz5500 *wiz, uint8_t socket, uint16_t offset, uint8_t *data, uint16_t data_len, uint8_t dma)
{
  // Get RX read ptr
  uint16_t rx_rd_ptr = _read_register16_for_sure(wiz, SOCKET_RX_READ_POINTER, REGISTER_SOCKET_N(socket));
  rx_rd_ptr += offset;

  // Update RX read ptr: current + offset + data_len
  int res = _write_register16(wiz, SOCKET_RX_READ_POINTER, REGISTER_SOCKET_N(socket), rx_rd_ptr + data_len);
  if (res != HAL_OK) {
    return res;
  }

  // Receive data
  uint8_t control[3] = {
    rx_rd_ptr >> 8,
    rx_rd_ptr & 0xff,
    ((REGISTER_SOCKET_N(socket) + REGISTER_SOCKET_RX) << 3) | DATA_MODE_VARIABLE | DATA_READ,
  };
  HAL_GPIO_WritePin(wiz->nss_port, wiz->nss_pin, GPIO_PIN_RESET);
  res = HAL_SPI_Transmit(wiz->spi, control, sizeof(control), wiz->spi_timeout);
  if (res != HAL_OK) {
    return res;
  }
  if (dma) {
    // Do not disable SPI - must be done by following call of *dma_complete()
    return HAL_SPI_Receive_DMA(wiz->spi, data, data_len);
  } else {
    res = HAL_SPI_Receive(wiz->spi, data, data_len, wiz->spi_timeout);
    HAL_GPIO_WritePin(wiz->nss_port, wiz->nss_pin, GPIO_PIN_SET);
  }

  return res;
}



uint8_t wiz5500_udp_open_socket(wiz5500 *wiz, uint8_t socket, uint16_t source_port)
{
  assert_param(socket <= MAX_SOCKET_NUMBER);

  // Set socket mode to P1 - UDP. Multicast / broadcast disabled.
  // Also unicast blocking disabled as well
  _write_register8(wiz, SOCKET_MODE, REGISTER_SOCKET_N(socket), 0x02);

  // Set source port
  uint8_t res = _write_register16(wiz, SOCKET_SOURCE_PORT, REGISTER_SOCKET_N(socket), source_port);
  if (res != HAL_OK) {
    return res;
  }

  return _write_register8(wiz, SOCKET_COMMAND, REGISTER_SOCKET_N(socket), SOCKET_COMMAND_OPEN);
}

uint8_t wiz5500_udp_sendto_blocking(wiz5500 *wiz, uint8_t socket, uint32_t dst_ip, uint16_t dst_port, uint8_t *data, uint16_t data_size)
{
  // Set DST IP / port
  uint8_t res = wiz5500_set_socket_dst_ip(wiz, socket, dst_ip);
  if (res != HAL_OK) {
    return res;
  }
  res = wiz5500_set_socket_dst_port(wiz, socket, dst_port);
  if (res != HAL_OK) {
    return res;
  }

  // Copy data into wiz5500
  tx_data(wiz, socket, data, data_size, 0);
  // Send packet
  _write_register8(wiz, SOCKET_COMMAND, REGISTER_SOCKET_N(socket), SOCKET_COMMAND_SEND);

  // Wait until packet sent
  uint8_t ints = 0;
  while (!(ints = wiz5500_socket_read_interrupt(wiz, socket))) {
    HAL_Delay(1);
  }

  return ints & WIZ5500_SOCKET_INT_SENDOK ? WIZ5500_OK : WIZ5500_ERROR;
}

uint8_t wiz5500_udp_sendto_dma_start(wiz5500 *wiz, uint8_t socket, uint32_t dst_ip, uint16_t dst_port, uint8_t *data, uint16_t data_size)
{
  // Set DST IP / port in NON DMA mode
  uint8_t res = wiz5500_set_socket_dst_ip(wiz, socket, dst_ip);
  if (res != HAL_OK) {
    return res;
  }
  res = wiz5500_set_socket_dst_port(wiz, socket, dst_port);
  if (res != HAL_OK) {
    return res;
  }

  // Copy data into wiz5500 using DMA
  return tx_data(wiz, socket, data, data_size, 1);
}

uint8_t wiz5500_udp_sendto_dma_complete(wiz5500 *wiz, uint8_t socket)
{
  // End previous SPI transaction (DMA)
  HAL_GPIO_WritePin(wiz->nss_port, wiz->nss_pin, GPIO_PIN_SET);

  // Send packet
  return _write_register8(wiz, SOCKET_COMMAND, REGISTER_SOCKET_N(socket), SOCKET_COMMAND_SEND);
}


uint16_t wiz5500_udp_received_size(wiz5500 *wiz, uint8_t socket)
{
  // In case of UDP mode each packet contains head of 8 bytes:
  // src / dst IP, so subtracting 8 bytes
  return _read_register16_for_sure(wiz, SOCKET_RX_RECEIVED_SIZE, REGISTER_SOCKET_N(socket)) - 8;
}

uint8_t wiz5500_receive_from_blocking(wiz5500 *wiz, uint8_t socket, uint8_t *data, uint16_t data_len)
{
  uint16_t len;

  while (!(len = wiz5500_udp_received_size(wiz, socket))) {
    HAL_Delay(1);
  }

  if (len > data_len) {
    len = data_len;
  }

  // Transfer data
  rx_data_offset(wiz, socket, 8, data, len, 0);
  // Complete recv
  _write_register8(wiz, SOCKET_COMMAND, REGISTER_SOCKET_N(socket), SOCKET_COMMAND_RECV);

  return len;
}

uint8_t wiz5500_receive_from_dma_start(wiz5500 *wiz, uint8_t socket, uint8_t *data, uint16_t data_len)
{
  uint16_t len = wiz5500_udp_received_size(wiz, socket);
  if (len == 0) {
    // No packet available
    return 0;
  }

  if (len > data_len) {
    len = data_len;
  }

  // Copy data from WIZ5500 using DMA
  return rx_data_offset(wiz, socket, 8, data, len, 1);
}

uint8_t wiz5500_receive_from_dma_complete(wiz5500 *wiz, uint8_t socket)
{
  // End previous SPI transaction (DMA)
  HAL_GPIO_WritePin(wiz->nss_port, wiz->nss_pin, GPIO_PIN_SET);

  // Complete receive - issue RECV command
  return _write_register8(wiz, SOCKET_COMMAND, REGISTER_SOCKET_N(socket), SOCKET_COMMAND_RECV);
}


uint8_t wiz5500_get_socket_status(wiz5500 *wiz, uint8_t socket)
{
  assert_param(socket <= MAX_SOCKET_NUMBER);

  return _read_register8(wiz, SOCKET_STATUS, REGISTER_SOCKET_N(socket));
}

uint16_t wiz5500_get_socket_source_port(wiz5500 *wiz, uint8_t socket)
{
  assert_param(socket <= MAX_SOCKET_NUMBER);

  return _read_register16(wiz, SOCKET_SOURCE_PORT, REGISTER_SOCKET_N(socket));
}

uint8_t  wiz5500_socket_enable_interrupt(wiz5500 *wiz, uint8_t socket, uint8_t ints)
{
  assert_param(socket <= MAX_SOCKET_NUMBER);

  // Add socket to global interrupt mask
  uint8_t value = _read_register8(wiz, COMMON_SOCKET_INTERRUPT_MASK, REGISTER_COMMON);
  value |= 1 << socket;
  _write_register8(wiz, COMMON_SOCKET_INTERRUPT_MASK, REGISTER_COMMON, value);

  // Configure socket's interrupts
  _write_register8(wiz, SOCKET_INTERRUPT_MASK, REGISTER_SOCKET_N(socket), ints);

  return WIZ5500_OK;
}

uint8_t  wiz5500_socket_read_interrupt(wiz5500 *wiz, uint8_t socket)
{
  assert_param(socket <= MAX_SOCKET_NUMBER);

  return _read_register8(wiz, SOCKET_INTERRUPT, REGISTER_SOCKET_N(socket));
}

uint8_t  wiz5500_socket_clear_interrupt(wiz5500 *wiz, uint8_t socket, uint8_t ints)
{
  assert_param(socket <= MAX_SOCKET_NUMBER);

  // Clear all socket interrupt flags
  _write_register8(wiz, SOCKET_INTERRUPT, REGISTER_SOCKET_N(socket), ints);
  // Clear global socket interrupt flag
  _write_register8(wiz, COMMON_SOCKET_INTERRUPT, REGISTER_COMMON, 1 << socket);

  return WIZ5500_OK;
}
