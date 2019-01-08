// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#ifndef __LORA_H
#define __LORA_H

#include "main.h"

#define MHZ                                1000000LLU
#define LORA_BASE_FREQUENCY_US             (915LLU*MHZ)

#define LORA_OK                            0
#define LORA_CRC_ERROR                     1
#define LORA_TIMEOUT                       2
#define LORA_INVALID_HEADER                3
#define LORA_ERROR                         4
#define LORA_BUSY                          5
#define LORA_EMPTY                         6

#define LORA_DEFAULT_TX_POWER              17

// TX power mode select
#define LORA_PA_OUTPUT_RFO                 0
#define LORA_PA_OUTPUT_PA_BOOST            1

// Coding rate
#define LORA_CODING_RATE_4_5               0x08
#define LORA_CODING_RATE_4_6               0x10
#define LORA_CODING_RATE_4_7               0x18
#define LORA_CODING_RATE_4_8               0x20

// Signal bandwidth
enum {
    LORA_BANDWIDTH_7_8_KHZ = 0,
    LORA_BANDWIDTH_10_4_KHZ,
    LORA_BANDWIDTH_15_6_KHZ,
    LORA_BANDWIDTH_20_8_KHZ,
    LORA_BANDWIDTH_31_25_KHZ,
    LORA_BANDWIDTH_41_7_KHZ,
    LORA_BANDWIDTH_62_5_KHZ,
    LORA_BANDWIDTH_125_KHZ,
    LORA_BANDWIDTH_250_KHZ,
    LORA_BANDWIDTH_500_KHZ,
    LORA_BW_LAST,
};

// LORA definition
typedef struct {
    // SPI parameters
    SPI_HandleTypeDef  *spi;
    GPIO_TypeDef       *nss_port;
    uint16_t            nss_pin;
    uint32_t            spi_timeout;
    // Operating frequency, in Hz
    uint32_t            frequency;
    // Output PIN (module internal, not related to your design)
    // Can be one of PA_OUTPUT_PA_BOOST / PA_OUTPUT_RFO
    uint32_t            pa_mode;

    // Base FIFO addresses for RX/TX
    uint8_t             tx_base_addr;
    uint8_t             rx_base_addr;
} lora_def;

// LORA Module setup //

// Initialize LORA with default parameters
uint8_t  lora_init(lora_def *lora, SPI_HandleTypeDef *spi, GPIO_TypeDef *nss_port,
                  uint16_t nss_pin, uint64_t freq);
uint8_t  lora_version(lora_def *lora);

// LORA mode selection //
void     lora_mode_sleep(lora_def *lora);
void     lora_mode_standby(lora_def *lora);
void     lora_mode_receive_continious(lora_def *lora);
void     lora_mode_receive_single(lora_def *lora);

// LORA signal / transmission parameters //
void     lora_set_tx_power(lora_def *lora, uint8_t level);
void     lora_set_frequency(lora_def *lora, uint64_t freq);
void     lora_set_signal_bandwidth(lora_def *lora, uint64_t bw);
uint64_t lora_get_signal_bandwidth(lora_def *lora);
void     lora_set_spreading_factor(lora_def *lora, uint8_t sf);
uint8_t  lora_get_spreading_factor(lora_def *lora);
void     lora_set_crc(lora_def *lora, uint8_t enable);
void     lora_set_coding_rate(lora_def *lora, uint8_t rate);
void     lora_set_preamble_length(lora_def *lora, uint16_t len);
void     lora_set_implicit_header_mode(lora_def *lora);
void     lora_set_explicit_header_mode(lora_def *lora);

// Received packet information //
uint8_t  lora_packet_rssi(lora_def *lora);
uint8_t  lora_packet_snr(lora_def *lora);

// SEND packet routines //
uint8_t  lora_is_transmitting(lora_def *lora);
uint8_t  lora_send_packet(lora_def *lora, uint8_t *data, uint8_t data_len);
uint8_t  lora_send_packet_blocking(lora_def *lora, uint8_t *data, uint8_t data_len, uint32_t timeout);

// RECEIVE packet routines //
uint8_t  lora_is_packet_available(lora_def *lora);
uint8_t  lora_receive_packet(lora_def *lora, uint8_t *buffer, uint8_t buffer_len, uint8_t *error);
uint8_t  lora_receive_packet_blocking(lora_def *lora, uint8_t *buffer, uint8_t buffer_len,
                                     uint32_t timeout, uint8_t *error);
void     lora_set_rx_symbol_timeout(lora_def *lora, uint16_t symbols);



#endif
