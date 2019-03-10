// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#ifndef __TEST_MOCKS__H
#define __TEST_MOCKS__H

std::string SPI_get_transmit_history_entry(size_t index);
void        SPI_queue_receive_data(const std::string& data);
void        SPI_clear_transmit_history();
void        SPI_clear_transmit_queue();

// I2C mock interface: check history / schedule data to be received
std::string I2C_get_transmit_history_entry(size_t index);
void        I2C_queue_receive_data(const std::string& data);
void        I2C_clear_transmit_history();
void        I2C_clear_transmit_queue();

// UART
std::string UART_get_transmit_history_entry(size_t index);
size_t      UART_get_transmit_history_size();
void        UART_clear_transmit_history();

#endif
