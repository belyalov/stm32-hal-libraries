# STM32 HAL libraries [![Build Status](https://travis-ci.org/belyalov/stm32-hal-libraries.svg?branch=master)](https://travis-ci.org/belyalov/stm32-hal-libraries)
Set of useful high quality libraries to make interaction with various hardware / modules / etc simple.

## List of libraries
- [LoRa library](https://github.com/belyalov/stm32-hal-libraries/blob/master/doc/lora.md) (for devices based on `SX1276` chip), **does not** implement LoRaWAN.
- [SHTC3 temperature/humidity](https://github.com/belyalov/stm32-hal-libraries/blob/master/doc/shtc3.md) no that high precision I2C sensor.
- [SHT3X temperature/humidity](https://github.com/belyalov/stm32-hal-libraries/blob/master/doc/sht3x.md) high precision I2C sensor.
- [SI7021 temperature/humidity](https://github.com/belyalov/stm32-hal-libraries/blob/master/doc/si7021.md) high precision I2C sensor.
- [VEML6030 ambient light](https://github.com/belyalov/stm32-hal-libraries/blob/master/doc/veml6030.md) high precision Ambient Light I2C sensor.
- **Debug** - tiny size helpers to print text / values through UART
- **Ring Buffer** - simple set of macros to work with ring (circular) buffer. In favour of \*nix `queue.h`.
- **NanoPB Ring Buffer streams** - ring buffer based input/output streams for nanopb.
- **Printf** - basic printf() redirector to hUARTx

## Usage
- Super simple way: just copy required files into your STM32 HAL project.. that's it!
- [git submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules)
