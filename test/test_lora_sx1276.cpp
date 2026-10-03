// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#include <gtest/gtest.h>
#include <initializer_list>
#include <string.h>
#include <vector>

#include "main.h"
extern "C" {
#include "lora_sx1276.h"
}
#include "test_mocks.h"

using namespace std;

// Register access as it appears on SPI bus
static string R(uint8_t addr)
{
  return string(1, (char)addr);
}

static string W(uint8_t addr, uint8_t value)
{
  return string{(char)(addr | 0x80), (char)value};
}

static vector<string> spi_history()
{
  vector<string> res;
  for (size_t i = 0; i < SPI_get_transmit_history_size(); i++) {
    res.push_back(SPI_get_transmit_history_entry(i));
  }
  return res;
}

// Values returned by register reads, in order
static void queue_reads(initializer_list<uint8_t> values)
{
  for (uint8_t value : values) {
    SPI_queue_receive_data(string(1, (char)value));
  }
}

class lora : public ::testing::Test {
protected:
  void SetUp() override {
    SPI_clear_transmit_history();
    SPI_clear_transmit_queue();
    memset(&radio, 0, sizeof(radio));
    radio.spi = &spi;
    radio.frequency = 902350000;
  }

  SPI_HandleTypeDef spi;
  lora_sx1276 radio;
};

TEST_F(lora, init_sequence)
{
  queue_reads({
    0x12,                    // version
    0x82, 0x02,              // image calibration: start, then done
    0x80,                    // LoRa mode / SLEEP
    0x72,                    // explicit header
    0xc3, 0x70,              // spreading factor
    0x72, 0x70, 0x00,        // low data rate optimize
    0x72,                    // coding rate
    0x20,                    // LNA
    0x72, 0xc3,              // bandwidth
    0x82, 0x70, 0x04,        // low data rate optimize
  });

  ASSERT_EQ(LORA_OK, lora_init_ex(&radio, &spi, NULL, 0, 902350000, 7, LORA_BANDWIDTH_250_KHZ, 5,
                                  LORA_PA_OUTPUT_PA_BOOST));

  ASSERT_EQ(spi_history(), (vector<string>{
    R(0x42),
    // FSK SLEEP: LongRangeMode can be changed only in SLEEP, so 2 writes
    W(0x01, 0x00), W(0x01, 0x00),
    // 902.35 MHz
    W(0x06, 0xe1), W(0x07, 0x96), W(0x08, 0x66),
    // Receiver calibration in FSK STDBY with PA cut, AutoImageCalOn cleared
    W(0x01, 0x01), W(0x09, 0x00), R(0x3b), W(0x3b, 0x42), R(0x3b),
    // LoRa SLEEP, again 2 writes, then verify
    W(0x01, 0x80), W(0x01, 0x80), R(0x01),
    // Explicit header
    R(0x1d), W(0x1d, 0x72),
    // SF7, keeping AutomaticIFOn
    R(0x31), W(0x31, 0xc3), W(0x37, 0x0a), R(0x1e), W(0x1e, 0x70),
    R(0x1d), R(0x1e), R(0x26), W(0x26, 0x04),
    // CR 4/5
    R(0x1d), W(0x1d, 0x72),
    // Preamble 8
    W(0x20, 0x00), W(0x21, 0x08),
    // LNA boost, AGC
    R(0x0c), W(0x0c, 0x23), W(0x26, 0x04),
    // 5 dBm on PA_BOOST, OCP 100mA
    W(0x4d, 0x84), W(0x0b, 0x2b), W(0x09, 0xf3),
    // BW 250 kHz with errata 2.1 / 2.3 settings
    R(0x1d), W(0x1d, 0x82), R(0x31), W(0x36, 0x03), W(0x31, 0x43), W(0x2f, 0x40), W(0x30, 0x00),
    R(0x1d), R(0x1e), R(0x26), W(0x26, 0x04),
    // STDBY
    W(0x01, 0x81),
  }));
  ASSERT_EQ(902350000U, radio.frequency);
  ASSERT_EQ(0U, radio.rx_frequency_offset);
}

TEST_F(lora, init_wrong_version)
{
  queue_reads({0x22});

  ASSERT_EQ(LORA_ERROR, lora_init(&radio, &spi, NULL, 0, 902350000));
}

TEST_F(lora, init_no_lora_mode)
{
  // Version, calibration start / done, then the radio stays in FSK SLEEP
  queue_reads({0x12, 0x82, 0x02, 0x00});

  ASSERT_EQ(LORA_ERROR, lora_init(&radio, &spi, NULL, 0, 902350000));
}

TEST_F(lora, init_calibration_timeout)
{
  queue_reads({0x12, 0x82});
  // ImageCalRunning never clears
  for (int i = 0; i < 21; i++) {
    queue_reads({0x22});
  }

  ASSERT_EQ(LORA_ERROR, lora_init(&radio, &spi, NULL, 0, 902350000));
}

TEST_F(lora, frequency)
{
  lora_set_frequency(&radio, 915000000);
  ASSERT_EQ(spi_history(), (vector<string>{W(0x06, 0xe4), W(0x07, 0xc0), W(0x08, 0x00)}));
  ASSERT_EQ(915000000U, radio.frequency);

  // Middle byte used to be lost
  SPI_clear_transmit_history();
  lora_set_frequency(&radio, 902350000);
  ASSERT_EQ(spi_history(), (vector<string>{W(0x06, 0xe1), W(0x07, 0x96), W(0x08, 0x66)}));

  // Rounded to the nearest 61.035 Hz step: 0xe1f999.6
  SPI_clear_transmit_history();
  lora_set_frequency(&radio, 903900000);
  ASSERT_EQ(spi_history(), (vector<string>{W(0x06, 0xe1), W(0x07, 0xf9), W(0x08, 0x9a)}));

  // 916.7 MHz: 0xe52ccc.8
  SPI_clear_transmit_history();
  lora_set_frequency(&radio, 916700000);
  ASSERT_EQ(spi_history(), (vector<string>{W(0x06, 0xe5), W(0x07, 0x2c), W(0x08, 0xcd)}));
}

TEST_F(lora, coding_rate)
{
  // BW 250 kHz, CR 4/5, implicit header: only CR bits change
  queue_reads({0x83});
  lora_set_coding_rate(&radio, LORA_CODING_RATE_4_8);
  ASSERT_EQ(spi_history(), (vector<string>{R(0x1d), W(0x1d, 0x89)}));

  SPI_clear_transmit_history();
  queue_reads({0x89});
  lora_set_coding_rate(&radio, LORA_CODING_RATE_4_5);
  ASSERT_EQ(spi_history(), (vector<string>{R(0x1d), W(0x1d, 0x83)}));
}

TEST_F(lora, preamble_length)
{
  lora_set_preamble_length(&radio, 1000);
  ASSERT_EQ(spi_history(), (vector<string>{W(0x20, 0x03), W(0x21, 0xe8)}));

  // Shortest is 6 symbols
  SPI_clear_transmit_history();
  lora_set_preamble_length(&radio, 2);
  ASSERT_EQ(spi_history(), (vector<string>{W(0x20, 0x00), W(0x21, 0x06)}));
}

TEST_F(lora, rx_symbol_timeout)
{
  // SF7, CRC on, timeout MSB from previous 300 symbols
  queue_reads({0x75});
  lora_set_rx_symbol_timeout(&radio, 100);
  ASSERT_EQ(spi_history(), (vector<string>{R(0x1e), W(0x1e, 0x74), W(0x1f, 0x64)}));

  // 10-bit value: clamped to 1023, CRC bit kept
  SPI_clear_transmit_history();
  queue_reads({0x74});
  lora_set_rx_symbol_timeout(&radio, 1024);
  ASSERT_EQ(spi_history(), (vector<string>{R(0x1e), W(0x1e, 0x77), W(0x1f, 0xff)}));
}

TEST_F(lora, packet_snr)
{
  queue_reads({0x28});
  ASSERT_EQ(10, lora_packet_snr(&radio));

  queue_reads({0xd8});
  ASSERT_EQ(-10, lora_packet_snr(&radio));

  // -10.75 dB
  queue_reads({0xd5});
  ASSERT_EQ(-11, lora_packet_snr(&radio));
}

TEST_F(lora, packet_rssi)
{
  // HF port, positive SNR
  queue_reads({60, 0x28});
  ASSERT_EQ(-97, lora_packet_rssi(&radio));

  // Below the noise floor: -157 + 20 - 10 dB SNR
  queue_reads({20, 0xd8});
  ASSERT_EQ(-147, lora_packet_rssi(&radio));

  // EU 865 MHz is still HF port
  radio.frequency = 865000000;
  queue_reads({60, 0x00});
  ASSERT_EQ(-97, lora_packet_rssi(&radio));

  // LF port
  radio.frequency = 433175000;
  queue_reads({60, 0x14});
  ASSERT_EQ(-104, lora_packet_rssi(&radio));

  ASSERT_EQ(R(0x1a), SPI_get_transmit_history_entry(0));
  ASSERT_EQ(R(0x19), SPI_get_transmit_history_entry(1));
}

TEST_F(lora, low_data_rate_optimize)
{
  // LDO is required for symbol duration over 16ms
  struct {
    uint8_t bandwidth;
    uint8_t sf;
    uint8_t mc3;
  } cases[] = {
    {LORA_BANDWIDTH_7_8_KHZ,  7,  0x0c},
    {LORA_BANDWIDTH_10_4_KHZ, 7,  0x04},
    {LORA_BANDWIDTH_62_5_KHZ, 9,  0x04},
    {LORA_BANDWIDTH_62_5_KHZ, 10, 0x0c},
    {LORA_BANDWIDTH_125_KHZ,  10, 0x04},
    {LORA_BANDWIDTH_125_KHZ,  11, 0x0c},
    {LORA_BANDWIDTH_250_KHZ,  12, 0x0c},
    {LORA_BANDWIDTH_500_KHZ,  12, 0x04},
  };

  for (auto &c : cases) {
    SCOPED_TRACE("bandwidth " + to_string(c.bandwidth) + ", sf " + to_string(c.sf));
    SPI_clear_transmit_history();
    queue_reads({0x43, 0x70, (uint8_t)(c.bandwidth << 4 | 0x02), (uint8_t)(c.sf << 4), 0x04});
    lora_set_spreading_factor(&radio, c.sf);
    vector<string> history = spi_history();
    ASSERT_EQ(W(0x26, c.mc3), history.back());
  }
}

TEST_F(lora, spreading_factor_6)
{
  queue_reads({0x43, 0x82, 0x70, 0x83, 0x60, 0x04});
  lora_set_spreading_factor(&radio, 6);

  ASSERT_EQ(spi_history(), (vector<string>{
    R(0x31), W(0x31, 0x45), W(0x37, 0x0c),
    // Implicit header
    R(0x1d), W(0x1d, 0x83),
    R(0x1e), W(0x1e, 0x60),
    R(0x1d), R(0x1e), R(0x26), W(0x26, 0x04),
  }));
}

TEST_F(lora, bandwidth_500_khz)
{
  queue_reads({0x82, 0x43, 0x92, 0x70, 0x04});
  lora_set_signal_bandwidth(&radio, LORA_BANDWIDTH_500_KHZ);

  // Errata 2.1 for HF band, AutomaticIFOn back on
  ASSERT_EQ(spi_history(), (vector<string>{
    R(0x1d), W(0x1d, 0x92), R(0x31), W(0x36, 0x02), W(0x3a, 0x64), W(0x31, 0xc3),
    R(0x1d), R(0x1e), R(0x26), W(0x26, 0x04),
  }));
}

TEST_F(lora, narrow_bandwidth_receiver_offset)
{
  radio.frequency = 433175000;
  queue_reads({0x72, 0xc3, 0x02, 0x70, 0x04});
  lora_set_signal_bandwidth(&radio, LORA_BANDWIDTH_7_8_KHZ);
  ASSERT_EQ(spi_history(), (vector<string>{
    R(0x1d), W(0x1d, 0x02), R(0x31), W(0x36, 0x03), W(0x31, 0x43), W(0x2f, 0x48), W(0x30, 0x00),
    // LO at nominal frequency
    W(0x06, 0x6c), W(0x07, 0x4b), W(0x08, 0x33),
    R(0x1d), R(0x1e), R(0x26), W(0x26, 0x0c),
  }));
  ASSERT_EQ(7810U, radio.rx_frequency_offset);

  // Errata 2.3: receive with LO moved by 7.81 kHz
  SPI_clear_transmit_history();
  lora_mode_receive_continuous(&radio);
  ASSERT_EQ(spi_history(), (vector<string>{
    W(0x0f, 0x00), W(0x12, 0xf0), W(0x06, 0x6c), W(0x07, 0x4b), W(0x08, 0xb3), W(0x01, 0x85),
  }));

  // Transmit at nominal frequency
  SPI_clear_transmit_history();
  queue_reads({0x85});
  uint8_t data[] = {1, 2, 3};
  ASSERT_EQ(LORA_OK, lora_send_packet(&radio, data, sizeof(data)));
  ASSERT_EQ(spi_history(), (vector<string>{
    R(0x01), W(0x01, 0x81), W(0x06, 0x6c), W(0x07, 0x4b), W(0x08, 0x33),
    W(0x12, 0x08), W(0x0d, 0x00), W(0x0e, 0x00), W(0x22, 0x03),
    // FIFO burst write
    string(1, '\x80'), string("\x01\x02\x03", 3),
    W(0x01, 0x83),
  }));
}

TEST_F(lora, receive_truncates_to_buffer)
{
  // RxDone without ValidHeader (cleared with the previous packet), explicit header,
  // 40 bytes at FIFO 0x10
  queue_reads({0x40, 0x82, 40, 0x10});
  SPI_queue_receive_data("ABCDEFGH");

  struct {
    uint8_t buf[8];
    uint8_t canary[8];
  } rx;
  memset(&rx, 0xee, sizeof(rx));
  uint8_t error = 0xff;
  ASSERT_EQ(8, lora_receive_packet(&radio, rx.buf, sizeof(rx.buf), &error));
  ASSERT_EQ(LORA_OK, error);
  ASSERT_EQ(0, memcmp(rx.buf, "ABCDEFGH", 8));
  ASSERT_EQ(string(8, '\xee'), string((char*)rx.canary, 8));

  // Only flags read get cleared
  ASSERT_EQ(spi_history(), (vector<string>{
    R(0x12), W(0x12, 0x40), R(0x1d), R(0x13), R(0x10), W(0x0d, 0x10), R(0x00),
  }));
}

TEST_F(lora, receive_crc_error)
{
  queue_reads({0x70});

  uint8_t buf[8];
  uint8_t error = 0xff;
  ASSERT_EQ(0, lora_receive_packet(&radio, buf, sizeof(buf), &error));
  ASSERT_EQ(LORA_CRC_ERROR, error);
  ASSERT_EQ(spi_history(), (vector<string>{R(0x12), W(0x12, 0x70)}));
}
