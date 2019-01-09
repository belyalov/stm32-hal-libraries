## Tiny LoRa for STM32 HAL
Simple and lightweight library to work with LoRa `sx1276` compatible modules on STM32 HAL.
Please be noticed - it **does not** implements LoRaWAN stack, it is pure LoRa - basically RADIO only.


### Quickstart
Copy / add as submodule files under `src` directory to your STM32 HAL project, then:
1. Include header:
```
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lora_sx1276.h"
// ...
/* USER CODE END Includes */
```
2. Add module initialization somewhere in `main()` function like that:
```
  /* USER CODE BEGIN 2 */
  lora_sx1276 lora;

  // SX1276 compatible module connected to SPI1, NSS pin connected to GPIO with label LORA_NSS
  uint8_t res = lora_init(&lora, &hspi1, LORA_NSS_GPIO_Port, LORA_NSS_Pin, LORA_BASE_FREQUENCY_US);
  if (res != LORA_OK) {
    // Initialization failed
  }
```
3. Send packet:
```
  // Send packet can be as simple as
  uint8_t res = lora_send_packet(&lora, (uint8_t *)"test", 4);
  if (res != LORA_OK) {
    // Send failed
  }
```
4. Receive packet:
```
  // Receive buffer
  uint8_t buffer[32];
  // Put LoRa modem into continuous receive mode
  lora_mode_receive_continious(&lora);
  // Wait for packet up to 10sec
  uint8_t res;
  uint8_t len = lora_receive_packet_blocking(&lora, buffer, sizeof(buffer), 10000, &res);
  if (res != LORA_OK) {
    // Receive failed
  }
  buffer[len] = 0;  // null terminate string to print it
  printf("'%s'\n", buffer);
```

### API
Very very soon! ) Meanwhile check `lora_sx1276.h` for inline comments.. )
