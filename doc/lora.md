# Tiny LoRa library for STM32 HAL
Simple and lightweight library to work with LoRa `sx1276` compatible modules on STM32 HAL.

It is really **tiny**, check this out:
```bash
$ arm-none-eabi-gcc -c -mcpu=cortex-m0plus -mthumb <other options stripped> lora_sx1276.c
$ size lora_sx1276.o
   text    data     bss     dec     hex filename
   1596       0       0    1596     63c lora_sx1276.o
   ```
It's just `1.5`Kb of compiled code! :)

Please be noticed - it **does not** implements LoRaWAN stack, it is pure LoRa - basically RADIO only.

## Quickstart
Copy / add as submodule files under `Src` directory to your STM32 HAL project, then:
1. Include header:
```cpp
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lora_sx1276.h"
// ...
/* USER CODE END Includes */
```
2. Add module initialization somewhere in `main()` function like that:
```cpp
  /* USER CODE BEGIN 2 */
  lora_sx1276 lora;

  // SX1276 compatible module connected to SPI1, NSS pin connected to GPIO with label LORA_NSS
  uint8_t res = lora_init(&lora, &hspi1, LORA_NSS_GPIO_Port, LORA_NSS_Pin, LORA_BASE_FREQUENCY_US);
  if (res != LORA_OK) {
    // Initialization failed
  }
```
3. Send packet:
```cpp
  // Send packet can be as simple as
  uint8_t res = lora_send_packet(&lora, (uint8_t *)"test", 4);
  if (res != LORA_OK) {
    // Send failed
  }
```
4. Receive packet:
```cpp
  // Receive buffer
  uint8_t buffer[32];
  // Put LoRa modem into continuous receive mode
  lora_mode_receive_continuous(&lora);
  // Wait for packet up to 10sec
  uint8_t res;
  uint8_t len = lora_receive_packet_blocking(&lora, buffer, sizeof(buffer), 10000, &res);
  if (res != LORA_OK) {
    // Receive failed
  }
  buffer[len] = 0;  // null terminate string to print it
  printf("'%s'\n", buffer);
```

## API

### LoRa initialization

* `uint8_t lora_init(lora_sx1276 *lora, SPI_HandleTypeDef *spi, GPIO_TypeDef *nss_port, uint16_t nss_pin, uint64_t freq)`
  Initialize LoRa with default parameters:
   - `lora` LoRa definition to be initialized
   - `spi` SPI HAL bus (`hspi1`, `hspi2`, etc)
   - `nss_port` - GPIO port where `NSS` pin connected to
   - `nss_pin` - GPIO pin number in `nss_port`
   - `freq` - operating frequency. In Hz
  Returns:
   - `LORA_OK` - modem initialized successfully
   - `LORA_ERROR` - initialization failed (e.g. no modem present on SPI bus / wrong NSS port/pin)


 * `uint8_t  lora_version(lora_sx1276 *lora)`
  Returns LoRa modem version number (usually `0x12`)

### LoRa mode selection

 * `void lora_mode_sleep(lora_sx1276 *lora)`
   Put radio into SLEEP mode: In this mode only SPI and configuration registers are accessible. LoRa FIFO is not accessible.

 * `void lora_mode_standby(lora_sx1276 *lora)`
   Put radio into standby (idle) mode: Both Crystal Oscillator and LoRa baseband blocks are turned on. RF part and PLLs are disabled.

 * `void lora_mode_receive_continuous(lora_sx1276 *lora)`
   Put radio into continuous receive mode: When activated the RFM95/96/97/98(W) powers all remaining blocks required for reception, processing all received data until a new user request is made to change operating mode.

 * `void lora_mode_receive_single(lora_sx1276 *lora)`
   Put radio into single receive mode:
   When activated the RFM95/96/97/98(W) powers all remaining blocks required for reception, remains in this state until a valid packet has been received and then returns to Standby mode.


### LoRa signal / transmission parameters

 * `void lora_set_tx_power(lora_sx1276 *lora, uint8_t level)`
   Sets LoRa transmit power.
   - `level` - TX power in dBm. Valid range from 2dBm to 20dBm

 * `void lora_set_frequency(lora_sx1276 *lora, uint64_t freq);`
   Set operational frequency.
   - `freq` - frequency in Hz

 * `void lora_set_signal_bandwidth(lora_sx1276 *lora, uint64_t bw)`
   Set signal bandwidth.
   - `bw` - desired bandwidth, from `LORA_BANDWIDTH_7_8_KHZ` to `LORA_BANDWIDTH_500_KHZ`
   For more information refer to section 4.1 of datasheet.

 * `void lora_set_spreading_factor(lora_sx1276 *lora, uint8_t sf)`
   Set signal spreading factor.
   - `sf` - spreading factor. Value from 6 to 12
   For more information refer to section 4.1 of datasheet.

 * `void lora_set_coding_rate(lora_sx1276 *lora, uint8_t rate)`
   Set coding rate.
   - `rate` - coding rate. Use any of `LORA_CODING_RATE*` constants.
   For more information refer to section 4.1 of datasheet.

 * `void lora_set_crc(lora_sx1276 *lora, uint8_t enable)`
   Enable / disable CRC
   - `enable` - set to `0` to disable CRC, any other value enables CRC.

 * `void lora_set_preamble_length(lora_sx1276 *lora, uint16_t len)`
   Set length of packet preamble.
   - `len` - length of packet preamble
   For more information refer to section 4.1.1.6 of datasheet

 * `void lora_set_implicit_header_mode(lora_sx1276 *lora)`
   Set "implicit header" mode, meaning no packet header at all.
   Refer to section 4.1.1.6 of datasheet

 * `void lora_set_explicit_header_mode(lora_sx1276 *lora)`
   Set "explicit", i.e. always add packet header with various system information.
   Refer to section 4.1.1.6 of datasheet

 * `void lora_enable_interrupt_rx_done(lora_sx1276 *lora)`
   Enables interrupt on DIO0 when packet received
   SX1276 module will pull DIO0 line high

 * `void lora_enable_interrupt_tx_done(lora_sx1276 *lora)`
   Enables interrupt on DIO0 when transmission is done
   SX1276 module will pull DIO0 line high

 * `void lora_clear_interrupt_tx_done(lora_sx1276 *lora)`
   Clears TX interrupt on DIO0

### Received packet information

 * `uint8_t lora_packet_rssi(lora_sx1276 *lora)`
   Returns RSSI of last received packet

 * `uint8_t lora_packet_snr(lora_sx1276 *lora)`
   Returns SNR of last received packet


### SEND packet routines

 * `uint8_t  lora_is_transmitting(lora_sx1276 *lora)`
   Query modem for any ongoing packet transmission.
   Returns `0` if no active transmission present

 * `uint8_t  lora_send_packet_dma_start(lora_sx1276 *lora, uint8_t *data, uint8_t data_len)`
   Sends packet using DMA. You **must** call `lora_send_packet_dma_complete()` when DMA is done.
    - `data` - pointer to buffer to be transmitted
    - `data_len` - how many bytes to transmit from `data` buffer.
   Returns:
    - `LORA_BUSY` in case of active transmission ongoing
    - `LORA_OK` packet scheduled to be sent.

 * `void  lora_send_packet_dma_complete(lora_sx1276 *lora);`
   Completes DMA based send packet initiated by `lora_send_packet_dma_start()`

 * `uint8_t  lora_send_packet_blocking(lora_sx1276 *lora, uint8_t *data, uint8_t data_len, uint32_t timeout)`
   Send packet in non-blocking mode
    - `data` - pointer to buffer to be transmitted
    - `data_len` - how many bytes to transmit from `data` buffer.
   Returns:
    - `LORA_BUSY` in case of active transmission ongoing
    - `LORA_OK` packet scheduled to be sent.
       Check state with `lora_is_transmitting()` or by interrupt.

 * `uint8_t  lora_send_packet(lora_sx1276 *lora, uint8_t *data, uint8_t data_len)`
   Send packet and returns only when packet sent / error occured (blocking mode).
    - `data` - pointer to buffer to be transmitted
    - `data_len` - how many bytes to transmit from `data` buffer.
    - `timeout` - maximum wait time to finish transmission.
   Returns:
    - `LORA_BUSY` in case of active transmission ongoing
    - `LORA_TIMEOUT` packet wasn't transmitted in given time frame.
    - `LORA_OK` packet scheduled to be sent.


### RECEIVE packet routines

 * `uint8_t  lora_is_packet_available(lora_sx1276 *lora)`
   Checks if packet modem has packet awaiting to be received
   Returns `0` if no packet is available, or any positive integer in case packet is ready

 * `uint8_t  lora_pending_packet_length(lora_sx1276 *lora);`
   If modem has packet awaiting to be received - returns it's length.


 * `uint8_t  lora_receive_packet(lora_sx1276 *lora, uint8_t *buffer, uint8_t buffer_len, uint8_t *error)`
   Receives packet from LoRa modem
    - `buffer` - pointer to buffer where copy packet to.
    - `buffer_len` - length of `buffer`. If incoming packet greater than `buffer_len` it will be truncated to fit `buffer_len`.
    - `error` - pointer to `uint8_t` to store error. Can be `NULL` - so no error information will be stored.
   Returns actual packet length stored into `buffer`.

 * `uint8_t  lora_receive_packet_dma_start(lora_sx1276 *lora, uint8_t *buffer, uint8_t buffer_len, uint8_t *error);`
   Receives packet from LoRa modem using DMA mode. **You must call `lora_receive_packet_dma_complete() from DMA receive done callback**
    - `buffer` - pointer to buffer where copy packet to.
    - `buffer_len` - length of `buffer`. If incoming packet greater than `buffer_len` it will be truncated to fit `buffer_len`.
    - `error` - pointer to `uint8_t` to store error. Can be `NULL` - so no error information will be stored.
   Returns actual packet length stored into `buffer`.

   `error` is one of:
    - `LORA_OK` - packet successfully received.
    - `LORA_EMPTY` - no packet received at the moment (check for packet by `lora_is_packet_available()` before).
    - `LORA_TIMEOUT` - timeout while receiving packet (only for single receive mode).
    - `LORA_INVALID_HEADER` - packet with malformed header received.
    - `LORA_CRC_ERROR` - malformed packet received (CRC failed). Please note that you need to enable this functionality explicitly, it is disabled by default.

 * `void  lora_receive_packet_dma_complete(lora_sx1276 *lora);`
   Completes DMA based receive packet initiated by `lora_receive_packet_dma_start()`

 * `uint8_t  lora_receive_packet_blocking(lora_sx1276 *lora, uint8_t *buffer, uint8_t buffer_len, uint32_t timeout, uint8_t *error)`
   Receive packet in "blocking mode" i.e. function return only when packet:
   1. In case of single receive mode: when packet arrived or timeout occurred.
      Please note that it is not enough to set `timeout` to something positive - you also need to set timeout in LoRa modem by calling `lora_set_rx_symbol_timeout` before.
   2. For continuous receiving mode will wait until packet arrived / timeout occurred.
    - `buffer` - pointer to buffer where copy packet to.
    - `buffer_len` - length of `buffer`. If incoming packet greater than `buffer_len` it will be truncated to fit `buffer_len`.
    - `error` - pointer to `uint8_t` to store error. Can be `NULL` - so no error information will be stored.
   Returns actual packet length stored into `buffer`.

   `error` is one of:
    - `LORA_OK` - packet successfully received.
    - `LORA_EMPTY` - no packet received at the moment (check for packet by `lora_is_packet_available()` before).
    - `LORA_TIMEOUT` - timeout while receiving packet (only for single receive mode).
    - `LORA_INVALID_HEADER` - packet with malformed header received.
    - `LORA_CRC_ERROR` - malformed packet received (CRC failed). Please note that you need to enable this functionality explicitly, it is disabled by default.

 * `void lora_set_rx_symbol_timeout(lora_sx1276 *lora, uint16_t symbols)`
   Sets timeout for `lora_mode_receive_single()` in symbols.
    - `symbols` - timeout value. Valid from `4` to `1024` symbols.
   For more information refer to datasheet section 4.1.5
