# SHTC3 library for STM32 HAL
Just essentials, nothing else! :)

```bash
$ arm-none-eabi-size shtc3.o
   text	   data	    bss	    dec	    hex	filename
    446	      0	      0	    446	    1be	shtc3.o
```
That's only `446b` :)

## Quickstart
Copy / add as submodule files under `Src` directory to your STM32 HAL project, then:
1. Include header:
```cpp
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "shtc3.h"
...
/* USER CODE END Includes */
```
2. Get your measurements! :)
```cpp
  // Somewhere in your main():

  if (!shtc3_read_id(&hi2c1)) {
    // Sensor is not found
  }

  int32_t temp;
  int32_t hum;
  if (shtc3_perform_measurements(&hi2c1, &temp, &hum)) {
    // Do something with values
  }
```

## API

 * `uint16_t shtc3_read_id(I2C_HandleTypeDef *hi2c)`

   Function to read shtc3 sensor serial number.
   - `hi2c` I2C bus

   Returns sensor id or 0 in case of error.

 * `uint32_t shtc3_sleep(I2C_HandleTypeDef *hi2c)`

   Put sensor into sleep mode
   - `hi2c` I2C bus

   Returns zero in case of error

 * `uint32_t shtc3_wakeup(I2C_HandleTypeDef *hi2c)`

   Wake up sensor.
   You must wait for 240us to let sensor enter into IDLE mode.
    - `hi2c` I2C bus

   Returns zero in case of error

 * `uint32_t shtc3_perform_measurements(I2C_HandleTypeDef *hi2c, int32_t* temp, int32_t* hum)`

   Performs full cycle: starts temperature/humidity measurements using "clock stretch" method.
    - `hi2c` I2C bus
    - `temp` measured temperature, in C multiplied by 100 (e.g. 24.1C -> 2410)
    - `hum` measured relative humidity, in percents

   Returns zero in case of error

 * `uint32_t shtc3_perform_measurements_low_power(I2C_HandleTypeDef *hi2c, int32_t* out_temp, int32_t* out_hum)`

   Start temperature/humidity measurements using "clock stretch" approach, in low power mode.
   After completed - values can be obtained by shtc3_read_measurements()
    - `hi2c` I2C bus
    - `temp` measured temperature, in C multiplied by 100 (e.g. 24.1C -> 2410)
    - `hum` measured relative humidity, in percents

   Returns zero in case of error
