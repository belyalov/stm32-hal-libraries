// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.
#include "lora_sx1276.h"
#include <stdio.h>

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
// Named as in the errata note / Semtech driver. The datasheet register map
// swaps the names (0x2f IfFreq2, 0x30 IfFreq1); values written follow the errata.
#define REG_IF_FREQ_1            0x2f
#define REG_IF_FREQ_2            0x30
#define REG_DETECTION_OPTIMIZE   0x31
#define REG_HIGH_BW_OPTIMIZE_1   0x36
#define REG_DETECTION_THRESHOLD  0x37
#define REG_HIGH_BW_OPTIMIZE_2   0x3a
#define REG_IMAGE_CAL            0x3b  // FSK mode only (RegInvertIQ2 in LoRa mode)
#define REG_DIO_MAPPING_1        0x40
#define REG_VERSION              0x42
#define REG_PA_DAC               0x4d

// modes
#define OPMODE_SLEEP             0x00
#define OPMODE_STDBY             0x01
#define OPMODE_TX                0x03
#define OPMODE_RX_CONTINUOUS     0x05
#define OPMODE_RX_SINGLE         0x06
#define OPMODE_MASK              0x07
#define OPMODE_LONG_RANGE_MODE   0x80  // (1 << 7)

// Power Amplifier (PA_DAC) settings
#define PA_DAC_HIGH_POWER        0x87
#define PA_DAC_HALF_POWER        0x84

// Over Current Protection (OCP) config
#define OCP_ON                      (1 << 5)

// Modem config register parameters
#define MC1_IMPLICIT_HEADER_MODE    (1 << 0)
#define MC1_CODING_RATE_MASK        0x0e

#define MC2_CRC_ON                  (1 << 2)
#define MC2_SYMB_TIMEOUT_MSB_MASK   0x03

#define MC3_AGCAUTO                 (1 << 2)
#define MC3_LOW_DATA_RATE_OPTIMIZE  (1 << 3)

// LoRa detection optimize register
#define DETECTION_AUTOMATIC_IF_ON   (1 << 7)
#define DETECTION_OPTIMIZE_MASK     0x07

// Image calibration register (FSK mode)
#define IMAGE_CAL_AUTO_ON           (1 << 7)
#define IMAGE_CAL_START             (1 << 6)
#define IMAGE_CAL_RUNNING           (1 << 5)
#define IMAGE_CAL_TIMEOUT           20  // ms, calibration takes ~10ms

// RSSI offsets for High / Low Frequency RF ports, datasheet section 5.5.5.
// LF port serves bands up to 525 MHz, HF port the band from 779 / 862 MHz.
#define RSSI_OFFSET_HF              -157
#define RSSI_OFFSET_LF              -164
#define LF_PORT_MAX_FREQUENCY       (525 * MHZ)

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

// Signal bandwidth as a divider of 500 kHz, indexed by LORA_BANDWIDTH_*
static const uint8_t bandwidth_div[LORA_BW_LAST] = {64, 48, 32, 24, 16, 12, 8, 4, 2, 1};

// Errata 2.3 "Receiver Spurious Reception of a LoRa Signal", for bandwidths below 500 kHz:
// RegIfFreq1 value and receiver LO offset, since narrow bandwidths move the IF.
static const struct {
  uint8_t  if_freq;
  uint16_t rx_offset;  // Hz
} spurious_rx_fix[LORA_BANDWIDTH_500_KHZ] = {
  {0x48, 7810},   // 7.8 kHz
  {0x44, 10420},  // 10.4 kHz
  {0x44, 15620},  // 15.6 kHz
  {0x44, 20830},  // 20.8 kHz
  {0x44, 31250},  // 31.25 kHz
  {0x44, 41670},  // 41.7 kHz
  {0x40, 0},      // 62.5 kHz
  {0x40, 0},      // 125 kHz
  {0x40, 0},      // 250 kHz
};

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

// Writes RF carrier frequency registers
static void write_frequency(lora_sx1276 *lora, uint64_t freq)
{
  // From datasheet: FREQ = (FRF * 32 Mhz) / (2 ^ 19)
  // FRF is 24-bit, round it to the nearest step (61.035 Hz)
  uint32_t frf = (uint32_t)(((freq << 19) + 16 * MHZ) / (32 * MHZ)) & 0xFFFFFF;

  write_register(lora, REG_FRF_MSB, (uint8_t)(frf >> 16));
  write_register(lora, REG_FRF_MID, (uint8_t)(frf >> 8));
  // New frequency is taken into account when LSB gets written
  write_register(lora, REG_FRF_LSB, (uint8_t)frf);
}

// Image and RSSI calibration of the receiver (datasheet 2.1.3.8).
// The automatic one done on POR covers only the LF port at 434 MHz,
// so it has to be repeated at the operating frequency.
// Works in FSK mode only: expects the radio in FSK SLEEP. Takes ~10ms.
static uint8_t calibrate_rx_chain(lora_sx1276 *lora)
{
  // Calibration must be started in STDBY, let crystal oscillator start (250us typ)
  write_register(lora, REG_OP_MODE, OPMODE_STDBY);
  HAL_Delay(1);
  // Cut the PA just in case, as Semtech's reference driver does.
  // Output power is set back by lora_set_tx_power().
  write_register(lora, REG_PA_CONFIG, 0x00);

  // Also turn off the temperature triggered re-calibration: datasheet recommends
  // so, since it may run in the middle of a packet.
  uint8_t image_cal = read_register(lora, REG_IMAGE_CAL) & (uint8_t)~IMAGE_CAL_AUTO_ON;
  write_register(lora, REG_IMAGE_CAL, image_cal | IMAGE_CAL_START);
  for (uint32_t elapsed = 0; read_register(lora, REG_IMAGE_CAL) & IMAGE_CAL_RUNNING; elapsed++) {
    if (elapsed >= IMAGE_CAL_TIMEOUT) {
      return LORA_ERROR;
    }
    HAL_Delay(1);
  }

  return LORA_OK;
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

  // Read current signal bandwidth / Spreading Factor
  uint8_t bandwidth = (read_register(lora, REG_MODEM_CONFIG_1) >> 4) & 0x0F;
  uint8_t sf = (read_register(lora, REG_MODEM_CONFIG_2) >> 4) & 0x0F;

  uint8_t mc3 = read_register(lora, REG_MODEM_CONFIG_3);
  mc3 |= MC3_AGCAUTO;

  // LowDataRateOptimize is mandated when symbol duration exceeds 16ms (datasheet 4.1.1.6).
  // Symbol duration is 2^SF / BW, where BW = 500 kHz / div, so the condition is:
  //   2^SF * div / 500 kHz > 16ms  <=>  2^SF * div > 8000
  if (bandwidth < LORA_BW_LAST && ((1UL << sf) * bandwidth_div[bandwidth]) > 8000) {
    mc3 |= MC3_LOW_DATA_RATE_OPTIMIZE;
  } else {
    mc3 &= (uint8_t)~MC3_LOW_DATA_RATE_OPTIMIZE;
  }

  write_register(lora, REG_MODEM_CONFIG_3, mc3);
}

void lora_mode_sleep(lora_sx1276 *lora)
{
  assert_param(lora);

  set_mode(lora, OPMODE_SLEEP);
}

// Prepares radio to receive packets and switches it into given receive mode
static void set_receive_mode(lora_sx1276 *lora, uint8_t mode)
{
  // Update base FIFO address for incoming packets
  write_register(lora, REG_FIFO_RX_BASE_ADDR, lora->rx_base_addr);
  // Clear all RX related IRQs
  write_register(lora, REG_IRQ_FLAGS, IRQ_FLAGS_RX_ALL);
  // Errata 2.3: narrow bandwidths move receiver IF, so LO has to be moved too
  if (lora->rx_frequency_offset) {
    write_frequency(lora, lora->frequency + lora->rx_frequency_offset);
  }

  set_mode(lora, mode);
}

void lora_mode_receive_continuous(lora_sx1276 *lora)
{
  assert_param(lora);

  set_receive_mode(lora, OPMODE_RX_CONTINUOUS);
}

void lora_mode_receive_single(lora_sx1276 *lora)
{
  assert_param(lora);

  set_receive_mode(lora, OPMODE_RX_SINGLE);
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

void lora_set_tx_power(lora_sx1276 *lora, uint8_t level_dbm)
{
  assert_param(lora);

  if (lora->pa_mode == LORA_PA_OUTPUT_RFO) {
    // RFO pin
    assert_param(level_dbm <= 14);
    if (level_dbm > 14) {
      level_dbm = 14;
    }
    // RegPaConfig:
    //  PaSelect=0 (RFO)
    //  MaxPower=7 (0x70)
    //  OutputPower maps: Pout ≈ -1 + OutputPower  => OutputPower = Pout + 1
    write_register(lora, REG_PA_CONFIG, 0x70 | (level_dbm + 1)); // 0..15
    write_register(lora, REG_PA_DAC, PA_DAC_HALF_POWER);
  } else {
    // PA BOOST pin, from datasheet (Power Amplifier):
    //   Pout=17-(15-OutputPower)
    assert_param(level_dbm <= 20 && level_dbm >= 2);
    if (level_dbm > 20) {
      level_dbm = 20;
    }
    if (level_dbm < 2) {
      level_dbm = 2;
    }
    uint8_t out;
    if (level_dbm > 17) {
      // 18..20 dBm uses high power PA DAC
      write_register(lora, REG_PA_DAC, PA_DAC_HIGH_POWER);
      set_OCP(lora, 140);
      // OutputPower maps: Pout ≈ 5 + OutputPower  => OutputPower = Pout - 5
      out = (uint8_t)(level_dbm - 5); // 13..15 for 18..20 dBm
    } else {
      write_register(lora, REG_PA_DAC, PA_DAC_HALF_POWER);
      set_OCP(lora, 100); // 95–100mA typical for 17 dBm
      // OutputPower maps: Pout ≈ 2 + OutputPower  => OutputPower = Pout - 2
      out = (uint8_t)(level_dbm - 2); // 0..15 for 2..17 dBm
    }
    // Set PaSelect=1, MaxPower=7, OutputPower=out
    write_register(lora, REG_PA_CONFIG, 0x80 | 0x70 | (out & 0x0F));
  }
}

void lora_set_frequency(lora_sx1276 *lora, uint64_t freq)
{
  assert_param(lora);

  write_frequency(lora, freq);
  lora->frequency = freq;
}

int16_t lora_packet_rssi(lora_sx1276 *lora)
{
  assert_param(lora);

  int16_t rssi = read_register(lora, REG_PKT_RSSI_VALUE);
  int8_t  snr = lora_packet_snr(lora);

  rssi += lora->frequency > LF_PORT_MAX_FREQUENCY ? RSSI_OFFSET_HF : RSSI_OFFSET_LF;
  // Packet below the noise floor: PacketRssi is noise there, signal is weaker by SNR
  if (snr < 0) {
    rssi += snr;
  }

  return rssi;
}

int8_t lora_packet_snr(lora_sx1276 *lora)
{
  assert_param(lora);

  // Two's complement value, in 0.25dB steps. Round to the nearest dB.
  int8_t snr = (int8_t)read_register(lora, REG_PKT_SNR_VALUE);

  return (snr + (snr < 0 ? -2 : 2)) / 4;
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

  // Receiver settings from the errata note
  uint8_t detect = read_register(lora, REG_DETECTION_OPTIMIZE);
  uint32_t rx_offset = 0;
  if (bw >= LORA_BANDWIDTH_500_KHZ) {
    // 2.1: Sensitivity optimization with a 500 kHz bandwidth
    write_register(lora, REG_HIGH_BW_OPTIMIZE_1, 0x02);
    write_register(lora, REG_HIGH_BW_OPTIMIZE_2, lora->frequency > LF_PORT_MAX_FREQUENCY ? 0x64 : 0x7f);
    write_register(lora, REG_DETECTION_OPTIMIZE, detect | DETECTION_AUTOMATIC_IF_ON);
  } else {
    write_register(lora, REG_HIGH_BW_OPTIMIZE_1, 0x03);
    // 2.3: Receiver spurious reception of a LoRa signal
    write_register(lora, REG_DETECTION_OPTIMIZE, detect & (uint8_t)~DETECTION_AUTOMATIC_IF_ON);
    write_register(lora, REG_IF_FREQ_1, spurious_rx_fix[bw].if_freq);
    write_register(lora, REG_IF_FREQ_2, 0x00);
    rx_offset = spurious_rx_fix[bw].rx_offset;
  }
  // Receiver LO offset changed: put LO back to the nominal frequency,
  // receive mode applies the new offset
  if (lora->rx_frequency_offset != rx_offset) {
    lora->rx_frequency_offset = rx_offset;
    write_frequency(lora, lora->frequency);
  }

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

  // DetectionOptimize uses bits 0-2. Keep AutomaticIFOn (bit 7),
  // lora_set_signal_bandwidth() sets it as errata 2.3 requires.
  uint8_t detect = read_register(lora, REG_DETECTION_OPTIMIZE) & (uint8_t)~DETECTION_OPTIMIZE_MASK;
  if (sf == 6) {
    write_register(lora, REG_DETECTION_OPTIMIZE, detect | 0x05);
    write_register(lora, REG_DETECTION_THRESHOLD, 0x0c);
    // SF6 is possible only in implicit header mode
    lora_set_implicit_header_mode(lora);
  } else {
    write_register(lora, REG_DETECTION_OPTIMIZE, detect | 0x03);
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
  assert_param(lora && rate >= LORA_CODING_RATE_4_5 && rate <= LORA_CODING_RATE_4_8);

  rate &= MC1_CODING_RATE_MASK;
  if (rate < LORA_CODING_RATE_4_5) {
    rate = LORA_CODING_RATE_4_5;
  } else if (rate > LORA_CODING_RATE_4_8) {
    rate = LORA_CODING_RATE_4_8;
  }

  uint8_t mc1 = read_register(lora, REG_MODEM_CONFIG_1);

  // coding rate bits are 1-3 in modem config 1 register, keep bandwidth / header mode
  mc1 = (mc1 & (uint8_t)~MC1_CODING_RATE_MASK) | rate;
  write_register(lora, REG_MODEM_CONFIG_1, mc1);
}

void lora_set_preamble_length(lora_sx1276 *lora, uint16_t len)
{
  assert_param(lora && len >= 6);

  // Shortest preamble is 6 symbols (datasheet 4.1.1.6)
  if (len < 6) {
    len = 6;
  }

  write_register(lora, REG_PREAMBLE_MSB, len >> 8);
  write_register(lora, REG_PREAMBLE_LSB, len & 0xff);
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
  // Transmit at the nominal frequency: receive mode may have moved LO (errata 2.3)
  if (lora->rx_frequency_offset) {
    write_frequency(lora, lora->frequency);
  }

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
  assert_param(lora && symbols <= 1023 && symbols >= 4);

  // Timeout is 10-bit value
  if (symbols < 4) {
    symbols = 4;
  }
  if (symbols > 1023) {
    symbols = 1023;
  }

  // MSB (2 first bits of config2), keep the rest of config2
  uint8_t mc2 = read_register(lora, REG_MODEM_CONFIG_2);
  mc2 = (mc2 & (uint8_t)~MC2_SYMB_TIMEOUT_MSB_MASK) | (symbols >> 8);
  write_register(lora, REG_MODEM_CONFIG_2, mc2);
  write_register(lora, REG_SYMB_TIMEOUT_LSB, symbols & 0xff);
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

  // Read/Reset IRQs. Reset only those that have been read: in continuous mode
  // the next packet may be raising its flags already.
  uint8_t state = read_register(lora, REG_IRQ_FLAGS);
  write_register(lora, REG_IRQ_FLAGS, state & IRQ_FLAGS_RX_ALL);

  if (state & IRQ_FLAGS_RX_TIMEOUT) {
    DEBUGF("timeout");
    res = LORA_TIMEOUT;
    goto done;
  }

  // RxDone comes only after a valid header, so ValidHeader is not checked:
  // it may be already cleared together with the previous packet.
  if (state & IRQ_FLAGS_RX_DONE) {
    // Packet has been received
    if (state & IRQ_FLAGS_PAYLOAD_CRC_ERROR) {
      DEBUGF("CRC error");
      res = LORA_CRC_ERROR;
      goto done;
    }
    // Query for current header mode - implicit / explicit
    len = lora_pending_packet_length(lora);
    // Length comes from the air: never write beyond the buffer
    if (len > buffer_len) {
      len = buffer_len;
    }
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

uint8_t  lora_init_ex(lora_sx1276 *lora, SPI_HandleTypeDef *spi, GPIO_TypeDef *nss_port,
                   uint16_t nss_pin, uint64_t freq, uint8_t sf, uint64_t bw, uint8_t tx_power, uint8_t tx_power_mode)
{
  assert_param(lora && spi);

  // Init params with default values
  lora->spi = spi;
  lora->nss_port = nss_port;
  lora->nss_pin = nss_pin;
  lora->frequency = freq;
  lora->rx_frequency_offset = 0;
  lora->pa_mode = tx_power_mode;
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
  // LongRangeMode (FSK / LoRa) can be changed only in SLEEP - a write in any
  // other mode is ignored, so enter SLEEP first, then select FSK.
  write_register(lora, REG_OP_MODE, OPMODE_SLEEP);
  write_register(lora, REG_OP_MODE, OPMODE_SLEEP);

  // Set frequency (common for FSK / LoRa modes)
  lora_set_frequency(lora, freq);

  // Receiver calibration works in FSK mode only
  if (calibrate_rx_chain(lora) != LORA_OK) {
    DEBUGF("Receiver calibration did not finish");
    return LORA_ERROR;
  }

  // Switch to LoRa the same way: enter SLEEP, then select LoRa
  lora_mode_sleep(lora);
  lora_mode_sleep(lora);
  uint8_t opmode = read_register(lora, REG_OP_MODE);
  if ((opmode & (OPMODE_LONG_RANGE_MODE | OPMODE_MASK)) != (OPMODE_LONG_RANGE_MODE | OPMODE_SLEEP)) {
    DEBUGF("Unable to enter LoRa mode, RegOpMode 0x%x", opmode);
    return LORA_ERROR;
  }

  // Explicit header mode (SF6 switches it to implicit)
  lora_set_explicit_header_mode(lora);
  lora_set_spreading_factor(lora, sf);
  lora_set_coding_rate(lora, LORA_DEFAULT_CR);
  lora_set_preamble_length(lora, LORA_DEFAULT_PREAMBLE_LEN);
  // Set LNA boost
  uint8_t current_lna = read_register(lora, REG_LNA);
  write_register(lora, REG_LNA,  current_lna | 0x03);
  // Set auto AGC
  write_register(lora, REG_MODEM_CONFIG_3, 0x04);
  // Set output power
  lora_set_tx_power(lora, tx_power);
  // Set signal bandwidth
  lora_set_signal_bandwidth(lora, bw);
  // Enter standby mode
  lora_mode_standby(lora);

  return LORA_OK;

}


uint8_t lora_init(lora_sx1276 *lora, SPI_HandleTypeDef *spi, GPIO_TypeDef *nss_port,
    uint16_t nss_pin, uint64_t freq)
{
  return lora_init_ex(lora, spi, nss_port, nss_pin, freq, LORA_DEFAULT_SF,
    LORA_BANDWIDTH_125_KHZ, LORA_DEFAULT_TX_POWER, LORA_PA_OUTPUT_PA_BOOST);
}

