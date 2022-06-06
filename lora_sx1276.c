// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.
#include "lora_sx1276.h"

// sx1276 registers
#define REG_FIFO                 0x00
#define REG_OP_MODE              0x01
#define REG_FRF_MSB              0x06
#define REG_FRF_MID              0x07
#define REG_FRF_LSB              0x08
#define REG_PA_CONFIG            0x09
#define REG_OCP                  0x0b
#define REG_LNA                  0x0c
#define REG_FIFO_ADDR_PTR        0x0d
#define REG_FIFO_TX_BASE_ADDR    0x0e
#define REG_FIFO_RX_BASE_ADDR    0x0f
#define REG_FIFO_RX_CURRENT_ADDR 0x10
#define REG_IRQ_FLAGS            0x12
#define REG_RX_NB_BYTES          0x13
#define REG_PKT_SNR_VALUE        0x19
#define REG_PKT_RSSI_VALUE       0x1a
#define REG_MODEM_CONFIG_1       0x1d
#define REG_MODEM_CONFIG_2       0x1e
#define REG_SYMB_TIMEOUT_LSB     0x1f
#define REG_PREAMBLE_MSB         0x20
#define REG_PREAMBLE_LSB         0x21
#define REG_PAYLOAD_LENGTH       0x22
#define REG_MODEM_CONFIG_3       0x26
#define REG_DETECTION_OPTIMIZE   0x31
#define REG_DETECTION_THRESHOLD  0x37
#define REG_DIO_MAPPING_1        0x40
#define REG_VERSION              0x42
#define REG_PA_DAC               0x4d

// modes
#define OPMODE_SLEEP             0x00
#define OPMODE_STDBY             0x01
#define OPMODE_TX                0x03
#define OPMODE_RX_CONTINUOUS     0x05
#define OPMODE_RX_SINGLE         0x06
#define OPMODE_LONG_RANGE_MODE   0x80  // (1 << 7)

// Power Amplifier (PA_DAC) settings
#define PA_DAC_HIGH_POWER        0x87
#define PA_DAC_HALF_POWER        0x84

// Over Current Protection (OCP) config
#define OCP_ON                      (1 << 5)

// Modem config register parameters
#define MC1_IMPLICIT_HEADER_MODE    (1 << 0)

#define MC2_CRC_ON                  (1 << 2)

#define MC3_AGCAUTO                 (1 << 2)
#define MC3_MOBILE_NODE             (1 << 3)

// IRQs
#define IRQ_FLAGS_RX_TIMEOUT        (1 << 7)
#define IRQ_FLAGS_RX_DONE           (1 << 6)
#define IRQ_FLAGS_PAYLOAD_CRC_ERROR (1 << 5)
#define IRQ_FLAGS_VALID_HEADER      (1 << 4)
#define IRQ_FLAGS_TX_DONE           (1 << 3)
#define IRQ_FLAGS_CAD_DONE          (1 << 2)
#define IRQ_FLAGS_FHSSCHANGECHANNEL (1 << 1)
#define IRQ_FLAGS_CAD_DETECTED      (1 << 0)
#define IRQ_FLAGS_RX_ALL            0xf0

// Just to make it readable
#define BIT_7                       (1 << 7)

#define TRANSFER_MODE_DMA           1
#define TRANSFER_MODE_BLOCKING      2

// Debugging support
// To enable debug information add
// #define LORA_DEBUG
// to main.h
#ifdef LORA_DEBUG
#define DEBUGF(msg, ...)     printf(const char *fmt, ##__VA_ARGS__);
#else
#define DEBUGF(msg, ...)
#endif

// SPI helpers //

// Reads single register
static uint8_t read_register(lora_sx1276 *lora, uint8_t address)
{
  uint8_t value = 0;

  // 7bit controls read/write mode
  CLEAR_BIT(address, BIT_7);

  // Start SPI transaction
  HAL_GPIO_WritePin(lora->nss_port, lora->nss_pin, GPIO_PIN_RESET);
  // Transmit reg address, then receive it value
  uint32_t res1 = HAL_SPI_Transmit(lora->spi, &address, 1, lora->spi_timeout);
  uint32_t res2 = HAL_SPI_Receive(lora->spi, &value, 1, lora->spi_timeout);
  // End SPI transaction
  HAL_GPIO_WritePin(lora->nss_port, lora->nss_pin, GPIO_PIN_SET);

  if (res1 != HAL_OK || res2 != HAL_OK) {
    DEBUGF("SPI transmit/receive failed (%d %d)", res1, res2);
  }

  return value;
}

// Writes single register
static void write_register(lora_sx1276 *lora, uint8_t address, uint8_t value)
{
  // 7bit controls read/write mode
  SET_BIT(address, BIT_7);

  // Reg address + its new value
  uint16_t payload = (value << 8) | address;

  // Start SPI transaction, send address + value
  HAL_GPIO_WritePin(lora->nss_port, lora->nss_pin, GPIO_PIN_RESET);
  uint32_t res = HAL_SPI_Transmit(lora->spi, (uint8_t*)&payload, 2, lora->spi_timeout);
  // End SPI transaction
  HAL_GPIO_WritePin(lora->nss_port, lora->nss_pin, GPIO_PIN_SET);

  if (res != HAL_OK) {
    DEBUGF("SPI transmit failed: %d", res);
  }
}

// Copies bytes from buffer into radio FIFO given len length
static void write_fifo(lora_sx1276 *lora, uint8_t *buffer, uint8_t len, uint8_t mode)
{
  uint8_t address = REG_FIFO | BIT_7;

  // Start SPI transaction, send address
  HAL_GPIO_WritePin(lora->nss_port, lora->nss_pin, GPIO_PIN_RESET);
  uint32_t res1 = HAL_SPI_Transmit(lora->spi, &address, 1, lora->spi_timeout);
  if (mode == TRANSFER_MODE_DMA) {
    HAL_SPI_Transmit_DMA(lora->spi, buffer, len);
    // Intentionally leave SPI active - let DMA finish transfer
    return;
  }
  uint32_t res2 = HAL_SPI_Transmit(lora->spi, buffer, len, lora->spi_timeout);
  // End SPI transaction
  HAL_GPIO_WritePin(lora->nss_port, lora->nss_pin, GPIO_PIN_SET);

  if (res1 != HAL_OK || res2 != HAL_OK) {
    DEBUGF("SPI transmit failed");
  }
}

// Reads data "len" size from FIFO into buffer
static void read_fifo(lora_sx1276 *lora, uint8_t *buffer, uint8_t len, uint8_t mode)
{
  uint8_t address = REG_FIFO;

  // Start SPI transaction, send address
  HAL_GPIO_WritePin(lora->nss_port, lora->nss_pin, GPIO_PIN_RESET);
  uint32_t res1 = HAL_SPI_Transmit(lora->spi, &address, 1, lora->spi_timeout);
  uint32_t res2;
  if (mode == TRANSFER_MODE_DMA) {
    res2 = HAL_SPI_Receive_DMA(lora->spi, buffer, len);
    // Do not end SPI here - must be done externally when DMA done
  } else {
    res2 = HAL_SPI_Receive(lora->spi, buffer, len, lora->spi_timeout);
    // End SPI transaction
    HAL_GPIO_WritePin(lora->nss_port, lora->nss_pin, GPIO_PIN_SET);
  }

  if (res1 != HAL_OK || res2 != HAL_OK) {
    DEBUGF("SPI receive/transmit failed");
  }
}

static void set_mode(lora_sx1276 *lora, uint8_t mode)
{
  write_register(lora, REG_OP_MODE, OPMODE_LONG_RANGE_MODE | mode);
}

// Set Overload Current Protection
static void set_OCP(lora_sx1276 *lora, uint8_t imax)
{
  uint8_t value;

  // Minimum available current is 45mA, maximum 240mA
  // As per page 80 of datasheet
  if (imax < 45) {
    imax = 45;
  }
  if (imax > 240) {
    imax = 240;
  }

  if (imax < 130) {
    value = (imax - 45) / 5;
  } else {
    value = (imax + 30) / 10;
  }

  write_register(lora, REG_OCP, OCP_ON | value);
}

static void set_low_data_rate_optimization(lora_sx1276 *lora)
{
  assert_param(lora);

  // Read current signal bandwidth
  uint64_t bandwidth = read_register(lora, REG_MODEM_CONFIG_1) >> 4;
  // Read current spreading factor
  uint8_t  sf = read_register(lora, REG_MODEM_CONFIG_2) >> 4;

  uint8_t  mc3 = MC3_AGCAUTO;

  if (sf >= 11 && bandwidth == LORA_BANDWIDTH_125_KHZ) {
    mc3 |= MC3_MOBILE_NODE;
  }

  write_register(lora, REG_MODEM_CONFIG_3, mc3);
}

void lora_mode_sleep(lora_sx1276 *lora)
{
  assert_param(lora);

  set_mode(lora, OPMODE_SLEEP);
}

void lora_mode_receive_continuous(lora_sx1276 *lora)
{
  assert_param(lora);

  // Update base FIFO address for incoming packets
  write_register(lora, REG_FIFO_RX_BASE_ADDR, lora->rx_base_addr);
  // Clear all RX related IRQs
  write_register(lora, REG_IRQ_FLAGS, IRQ_FLAGS_RX_ALL);

  set_mode(lora, OPMODE_RX_CONTINUOUS);
}

void lora_mode_receive_single(lora_sx1276 *lora)
{
  assert_param(lora);

  // Update base FIFO address for incoming packets
  write_register(lora, REG_FIFO_RX_BASE_ADDR, lora->rx_base_addr);
  // Clear all RX related IRQs
  write_register(lora, REG_IRQ_FLAGS, IRQ_FLAGS_RX_ALL);

  set_mode(lora, OPMODE_RX_SINGLE);
}

void lora_mode_standby(lora_sx1276 *lora)
{
  assert_param(lora);

  set_mode(lora, OPMODE_STDBY);
}

void lora_set_implicit_header_mode(lora_sx1276 *lora)
{
  assert_param(lora);

  uint8_t mc1 = read_register(lora, REG_MODEM_CONFIG_1);
  mc1 |= MC1_IMPLICIT_HEADER_MODE;
  write_register(lora, REG_MODEM_CONFIG_1, mc1);
}

void lora_set_explicit_header_mode(lora_sx1276 *lora)
{
  assert_param(lora);

  uint8_t mc1 = read_register(lora, REG_MODEM_CONFIG_1);
  mc1 &= ~MC1_IMPLICIT_HEADER_MODE;
  write_register(lora, REG_MODEM_CONFIG_1, mc1);
}

void lora_set_tx_power(lora_sx1276 *lora, uint8_t level)
{
  assert_param(lora);

  if (lora->pa_mode == LORA_PA_OUTPUT_RFO) {
    // RFO pin
    assert_param(level <= 15);
    if (level > 15) {
      level = 15;
    }
    // 7 bit -> PaSelect: 0 for RFO    --- = 0x70
    // 6-4 bits -> MaxPower (select all) --^
    // 3-0 bits -> Output power, dB (max 15)
    write_register(lora, REG_PA_CONFIG, 0x70 | level);
  } else {
    // PA BOOST pin, from datasheet (Power Amplifier):
    //   Pout=17-(15-OutputPower)
    assert_param(level <= 20 && level >= 2);
    if (level > 20) {
      level = 20;
    }
    if (level < 2) {
      level = 2;
    }
    // Module power consumption from datasheet:
    // RFOP = +20 dBm, on PA_BOOST -> 120mA
    // RFOP = +17 dBm, on PA_BOOST -> 87mA
    if (level > 17) {
      // PA_DAC_HIGH_POWER operation changes last 3 OutputPower modes to:
      // 13 -> 18dB, 14 -> 19dB, 15 -> 20dB
      // So subtract 3 from level
      level -= 3;
      // Enable High Power mode
      write_register(lora, REG_PA_DAC, PA_DAC_HIGH_POWER);
      // Limit maximum current to 140mA (+20mA to datasheet value to be sure)
      set_OCP(lora, 140);
    } else {
      // Enable half power mode (default)
      write_register(lora, REG_PA_DAC, PA_DAC_HALF_POWER);
      // Limit maximum current to 97mA (+10mA to datasheet value to be sure)
      set_OCP(lora, 97);
    }
    // Minimum power level is 2 which is 0 for chip
    level -= 2;
    // 7 bit -> PaSelect: 1 for PA_BOOST
    write_register(lora, REG_PA_CONFIG, BIT_7 | level);
  }
}

void lora_set_frequency(lora_sx1276 *lora, uint64_t freq)
{
  assert_param(lora);

  // From datasheet: FREQ = (FRF * 32 Mhz) / (2 ^ 19)
  uint64_t frf = (freq << 19) / (32 * MHZ);

  write_register(lora, REG_FRF_MSB, frf >> 16);
  write_register(lora, REG_FRF_MID, (frf & 0xff00) >> 8);
  write_register(lora, REG_FRF_LSB, frf & 0xff);
}

int8_t lora_packet_rssi(lora_sx1276 *lora)
{
  assert_param(lora);

  uint8_t rssi = read_register(lora, REG_PKT_RSSI_VALUE);

  return lora->frequency < (868 * MHZ) ? rssi - 164 : rssi - 157;
}

uint8_t lora_packet_snr(lora_sx1276 *lora)
{
  assert_param(lora);

  uint8_t snr = read_register(lora, REG_PKT_SNR_VALUE);

  return snr / 5;
}

void lora_set_signal_bandwidth(lora_sx1276 *lora, uint64_t bw)
{
  assert_param(lora && bw < LORA_BW_LAST);

  // REG_MODEM_CONFIG_1 has 2 more parameters:
  // Coding rate / Header mode, so read them before set bandwidth
  uint8_t mc1 = read_register(lora, REG_MODEM_CONFIG_1);
  // Signal bandwidth uses 4-7 bits of config
  mc1 = (mc1 & 0x0F) | bw << 4;
  write_register(lora, REG_MODEM_CONFIG_1, mc1);

  set_low_data_rate_optimization(lora);
}

void lora_set_spreading_factor(lora_sx1276 *lora, uint8_t sf)
{
  assert_param(lora && sf <= 12 && sf >=6);

  if (sf < 6) {
    sf = 6;
  } else if (sf > 12) {
    sf = 12;
  }

  if (sf == 6) {
    write_register(lora, REG_DETECTION_OPTIMIZE, 0xc5);
    write_register(lora, REG_DETECTION_THRESHOLD, 0x0c);
  } else {
    write_register(lora, REG_DETECTION_OPTIMIZE, 0xc3);
    write_register(lora, REG_DETECTION_THRESHOLD, 0x0a);
  }
  // Set new spread factor
  uint8_t mc2 = read_register(lora, REG_MODEM_CONFIG_2);
  mc2 = (mc2 & 0x0F) | (sf << 4);
  // uint8_t new_config = (current_config & 0x0f) | ((sf << 4) & 0xf0);
  write_register(lora, REG_MODEM_CONFIG_2, mc2);

  set_low_data_rate_optimization(lora);
}

void lora_set_crc(lora_sx1276 *lora, uint8_t enable)
{
  assert_param(lora);

  uint8_t mc2 = read_register(lora, REG_MODEM_CONFIG_2);

  if (enable) {
    mc2 |= MC2_CRC_ON;
  } else {
    mc2 &= ~MC2_CRC_ON;
  }

  write_register(lora, REG_MODEM_CONFIG_2, mc2);
}

void lora_set_coding_rate(lora_sx1276 *lora, uint8_t rate)
{
  assert_param(lora);

  uint8_t mc1 = read_register(lora, REG_MODEM_CONFIG_1);

  // coding rate bits are 1-3 in modem config 1 register
  mc1 |= rate << 1;
  write_register(lora, REG_MODEM_CONFIG_1, mc1);
}

void lora_set_preamble_length(lora_sx1276 *lora, uint16_t len)
{
  assert_param(lora);

  write_register(lora, REG_PREAMBLE_MSB, len >> 8);
  write_register(lora, REG_PREAMBLE_LSB, len & 0xf);
}

uint8_t lora_version(lora_sx1276 *lora)
{
  assert_param(lora);

  return read_register(lora, REG_VERSION);
}

uint8_t lora_is_transmitting(lora_sx1276 *lora)
{
  assert_param(lora);

  uint8_t opmode = read_register(lora, REG_OP_MODE);

  return (opmode & OPMODE_TX) == OPMODE_TX ? LORA_BUSY : LORA_OK;
}

static uint8_t lora_send_packet_base(lora_sx1276 *lora, uint8_t *data, uint8_t data_len, uint8_t mode)
{
  assert_param(lora && data && data_len > 0);

  if (lora_is_transmitting(lora)) {
    return LORA_BUSY;
  }

  // Wakeup radio because of FIFO is only available in STANDBY mode
  set_mode(lora, OPMODE_STDBY);

  // Clear TX IRQ flag, to be sure
  lora_clear_interrupt_tx_done(lora);

  // Set FIFO pointer to the beginning of the buffer
  write_register(lora, REG_FIFO_ADDR_PTR, lora->tx_base_addr);
  write_register(lora, REG_FIFO_TX_BASE_ADDR, lora->tx_base_addr);
  write_register(lora, REG_PAYLOAD_LENGTH, data_len);

  // Copy packet into radio FIFO
  write_fifo(lora, data, data_len, mode);
  if (mode == TRANSFER_MODE_DMA) {
    return LORA_OK;
  }

  // Put radio in TX mode - packet will be transmitted ASAP
  set_mode(lora, OPMODE_TX);
  return LORA_OK;
}

uint8_t lora_send_packet(lora_sx1276 *lora, uint8_t *data, uint8_t data_len)
{
  return lora_send_packet_base(lora, data, data_len, TRANSFER_MODE_BLOCKING);
}

uint8_t lora_send_packet_dma_start(lora_sx1276 *lora, uint8_t *data, uint8_t data_len)
{
  return lora_send_packet_base(lora, data, data_len, TRANSFER_MODE_DMA);
}

// Finish packet send initiated by lora_send_packet_dma_start()
void  lora_send_packet_dma_complete(lora_sx1276 *lora)
{
  // End transfer
  HAL_GPIO_WritePin(lora->nss_port, lora->nss_pin, GPIO_PIN_SET);
  // Send packet
  set_mode(lora, OPMODE_TX);
}

uint8_t lora_send_packet_blocking(lora_sx1276 *lora, uint8_t *data, uint8_t data_len, uint32_t timeout)
{
  assert_param(lora && data && data_len > 0 && timeout > 0);

  uint8_t res = lora_send_packet(lora, data, data_len);

  if (res == LORA_OK) {
    // Wait until packet gets transmitted
    uint32_t elapsed = 0;
    while (elapsed < timeout) {
      uint8_t state = read_register(lora, REG_IRQ_FLAGS);
      if (state & IRQ_FLAGS_TX_DONE) {
        // Packet sent
        write_register(lora, REG_IRQ_FLAGS, IRQ_FLAGS_TX_DONE);
        return LORA_OK;
      }
      HAL_Delay(1);
      elapsed++;
    }
  }

  return LORA_TIMEOUT;
}

void lora_set_rx_symbol_timeout(lora_sx1276 *lora, uint16_t symbols)
{
  assert_param(lora && symbols <= 1024 && symbols >= 4);

  if (symbols < 4) {
    symbols = 4;
  }
  if (symbols > 1023) {
    symbols = 1024;
  }

  write_register(lora, REG_SYMB_TIMEOUT_LSB, symbols & 0xf);
  if (symbols > 255) {
    // MSB (2 first bits of config2)
    uint8_t mc2 = read_register(lora, REG_MODEM_CONFIG_2);
    mc2 |= symbols >> 8;
    write_register(lora, REG_MODEM_CONFIG_2, mc2);
  }
}

uint8_t lora_is_packet_available(lora_sx1276 *lora)
{
  assert_param(lora);

  uint8_t irqs = read_register(lora, REG_IRQ_FLAGS);

  // In case of Single receive mode RX_TIMEOUT will be issued
  return  irqs & (IRQ_FLAGS_RX_DONE | IRQ_FLAGS_RX_TIMEOUT);
}

uint8_t lora_pending_packet_length(lora_sx1276 *lora)
{
  uint8_t len;

  // Query for current header mode - implicit / explicit
  uint8_t implicit = read_register(lora, REG_MODEM_CONFIG_1) & MC1_IMPLICIT_HEADER_MODE;
  if (implicit) {
    len = read_register(lora, REG_PAYLOAD_LENGTH);
  } else {
    len = read_register(lora, REG_RX_NB_BYTES);
  }

  return len;
}


static uint8_t lora_receive_packet_base(lora_sx1276 *lora, uint8_t *buffer, uint8_t buffer_len, uint8_t *error, uint8_t mode)
{
  assert_param(lora && buffer && buffer_len > 0);

  uint8_t res = LORA_EMPTY;
  uint8_t len = 0;

  // Read/Reset IRQs
  uint8_t state = read_register(lora, REG_IRQ_FLAGS);
  write_register(lora, REG_IRQ_FLAGS, IRQ_FLAGS_RX_ALL);

  if (state & IRQ_FLAGS_RX_TIMEOUT) {
    DEBUGF("timeout");
    res = LORA_TIMEOUT;
    goto done;
  }

  if (state & IRQ_FLAGS_RX_DONE) {
    if (!(state & IRQ_FLAGS_VALID_HEADER)) {
      DEBUGF("invalid header");
      res = LORA_INVALID_HEADER;
      goto done;
    }
    // Packet has been received
    if (state & IRQ_FLAGS_PAYLOAD_CRC_ERROR) {
      DEBUGF("CRC error");
      res = LORA_CRC_ERROR;
      goto done;
    }
    // Query for current header mode - implicit / explicit
    len = lora_pending_packet_length(lora);
    // Set FIFO to beginning of the packet
    uint8_t offset = read_register(lora, REG_FIFO_RX_CURRENT_ADDR);
    write_register(lora, REG_FIFO_ADDR_PTR, offset);
    // Read payload
    read_fifo(lora, buffer, len, mode);
    res = LORA_OK;
  }

done:
  if (error) {
    *error = res;
  }

  return len;
}

uint8_t lora_receive_packet(lora_sx1276 *lora, uint8_t *buffer, uint8_t buffer_len, uint8_t *error)
{
  return lora_receive_packet_base(lora, buffer, buffer_len, error, TRANSFER_MODE_BLOCKING);
}

uint8_t lora_receive_packet_dma_start(lora_sx1276 *lora, uint8_t *buffer, uint8_t buffer_len, uint8_t *error)
{
  return lora_receive_packet_base(lora, buffer, buffer_len, error, TRANSFER_MODE_DMA);
}

void lora_receive_packet_dma_complete(lora_sx1276 *lora)
{
  // Nothing to do expect - just end SPI transaction
  HAL_GPIO_WritePin(lora->nss_port, lora->nss_pin, GPIO_PIN_SET);
}

uint8_t lora_receive_packet_blocking(lora_sx1276 *lora, uint8_t *buffer, uint8_t buffer_len,
                   uint32_t timeout, uint8_t *error)
{
  assert_param(lora && buffer && buffer_len > 0);

  uint32_t elapsed = 0;

  // Wait up to timeout for packet
  while (elapsed < timeout) {
    if (lora_is_packet_available(lora)) {
      break;
    }
    HAL_Delay(1);
    elapsed++;
  }

  return lora_receive_packet(lora, buffer, buffer_len, error);
}

void lora_enable_interrupt_rx_done(lora_sx1276 *lora)
{
  // Table 63 DIO Mapping LoRaTM Mode:
  // 00 -> (DIO0 rx_done)
  // DIO0 uses 6-7 bits of DIO_MAPPING_1
  write_register(lora, REG_DIO_MAPPING_1, 0x00);
}

void lora_enable_interrupt_tx_done(lora_sx1276 *lora)
{
  // Table 63 DIO Mapping LoRaTM Mode:
  // 01 -> (DIO0 tx_done)
  // DIO0 uses 6-7 bits of DIO_MAPPING_1
  write_register(lora, REG_DIO_MAPPING_1, 0x40);
}

void lora_clear_interrupt_tx_done(lora_sx1276 *lora)
{
  write_register(lora, REG_IRQ_FLAGS, IRQ_FLAGS_TX_DONE);
}

void lora_clear_interrupt_rx_all(lora_sx1276 *lora)
{
  write_register(lora, REG_IRQ_FLAGS, IRQ_FLAGS_RX_ALL);
}


uint8_t lora_init(lora_sx1276 *lora, SPI_HandleTypeDef *spi, GPIO_TypeDef *nss_port,
    uint16_t nss_pin, uint64_t freq)
{
  assert_param(lora && spi);

  // Init params with default values
  lora->spi = spi;
  lora->nss_port = nss_port;
  lora->nss_pin = nss_pin;
  lora->frequency = freq;
  lora->pa_mode = LORA_PA_OUTPUT_PA_BOOST;
  lora->tx_base_addr = LORA_DEFAULT_TX_ADDR;
  lora->rx_base_addr = LORA_DEFAULT_RX_ADDR;
  lora->spi_timeout = LORA_DEFAULT_SPI_TIMEOUT;

  // Check version
  uint8_t ver = lora_version(lora);
  if (ver != LORA_COMPATIBLE_VERSION) {
    DEBUGF("Got wrong radio version 0x%x, expected 0x12", ver);
    return LORA_ERROR;
  }

  // Modem parameters (freq, mode, etc) must be done in SLEEP mode.
  lora_mode_sleep(lora);
  // Enable LoRa mode (since it can be switched on only in sleep)
  lora_mode_sleep(lora);

  // Set frequency
  lora_set_frequency(lora, freq);
  lora_set_spreading_factor(lora, LORA_DEFAULT_SF);
  lora_set_preamble_length(lora, LORA_DEFAULT_PREAMBLE_LEN);
  // By default - explicit header mode
  lora_set_explicit_header_mode(lora);
  // Set LNA boost
  uint8_t current_lna = read_register(lora, REG_LNA);
  write_register(lora, REG_LNA,  current_lna | 0x03);
  // Set auto AGC
  write_register(lora, REG_MODEM_CONFIG_3, 0x04);
  // Set default output power
  lora_set_tx_power(lora, LORA_DEFAULT_TX_POWER);
  // Set default mode
  lora_mode_standby(lora);

  return LORA_OK;
}

