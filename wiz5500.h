// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#ifndef __WIZ5500_H
#define __WIZ5500_H

#include "main.h"

// Error codes //
#define WIZ5500_OK              0
#define WIZ5500_ERROR           1
#define WIZ5500_TX_FULL         2

// Link status //
#define WIZ5500_LINK_DOWN       0
#define WIZ5500_LINK_UP         1

#define WIZ5500_LINK_AUTO       (0x7 << 3)
#define WIZ5500_LINK_PWRDOWN    (0x6 << 3)
#define WIZ5500_LINK_10HALF     (0x0 << 3)
#define WIZ5500_LINK_10FULL     (0x1 << 3)
#define WIZ5500_LINK_100HALF    (0x2 << 3)
#define WIZ5500_LINK_100FULL    (0x3 << 3)

// Socket status //
#define WIZ5500_SOCKET_CLOSED   0x00
#define WIZ5500_SOCKET_INIT     0x13
#define WIZ5500_SOCKET_LISTEN   0x14
#define WIZ5500_SOCKET_ESTABLISHED 0x17
#define WIZ5500_SOCKET_CLOSE_WAIT 0x1C
#define WIZ5500_SOCKET_UDP      0x22
#define WIZ5500_SOCKET_MAC_RAW  0x42


// Socket interrupts //
#define WIZ5500_SOCKET_INT_CON      (1 << 0)
#define WIZ5500_SOCKET_INT_DISCON   (1 << 1)
#define WIZ5500_SOCKET_INT_RECV     (1 << 2)
#define WIZ5500_SOCKET_INT_TIMEOUT  (1 << 3)
#define WIZ5500_SOCKET_INT_SENDOK   (1 << 4)
#define WIZ5500_SOCKET_INT_ALL      0x1F


// Wiz5500 params
typedef struct {
  SPI_HandleTypeDef  *spi;
  GPIO_TypeDef       *nss_port;
  uint16_t            nss_pin;
  uint32_t            spi_timeout;
} wiz5500;


uint32_t wiz5500_version(wiz5500 *wiz);
uint8_t wiz5500_ping_enable(wiz5500 *wiz);
uint8_t wiz5500_ping_disable(wiz5500 *wiz);
uint8_t wiz5500_link_status(wiz5500 *wiz);
uint8_t wiz5500_set_link(wiz5500 *wiz, uint8_t mode);
uint8_t wiz5500_is_link_full_duplex(wiz5500 *wiz);
uint8_t wiz5500_is_link_speed_100mb(wiz5500 *wiz);
uint32_t wiz5500_get_source_ip(wiz5500 *wiz);
uint8_t wiz5500_set_source_ip(wiz5500 *wiz, uint32_t ip);
uint64_t wiz5500_get_mac(wiz5500 *wiz);
uint64_t wiz5500_set_mac(wiz5500 *wiz, uint64_t mac);
uint8_t wiz5500_set_socket_mode(wiz5500 *wiz, uint8_t socket, uint8_t mode);

// UDP sockets //
uint8_t  wiz5500_udp_open_socket(wiz5500 *wiz, uint8_t socket, uint16_t source_port);
uint8_t  wiz5500_udp_sendto_blocking(wiz5500 *wiz, uint8_t socket, uint32_t dst_ip, uint16_t dst_port, uint8_t *data, uint16_t data_size);
uint8_t  wiz5500_udp_sendto_dma_start(wiz5500 *wiz, uint8_t socket, uint32_t dst_ip, uint16_t dst_port, uint8_t *data, uint16_t data_size);
uint8_t  wiz5500_udp_sendto_dma_complete(wiz5500 *wiz, uint8_t socket);
uint16_t wiz5500_udp_received_size(wiz5500 *wiz, uint8_t socket);
uint8_t  wiz5500_receive_from_blocking(wiz5500 *wiz, uint8_t socket, uint8_t *data, uint16_t data_len);
uint8_t wiz5500_receive_from_dma_start(wiz5500 *wiz, uint8_t socket, uint8_t *data, uint16_t data_len);
uint8_t  wiz5500_receive_from_dma_complete(wiz5500 *wiz, uint8_t socket);

// Common sockets //
uint8_t  wiz5500_get_socket_status(wiz5500 *wiz, uint8_t socket);

uint8_t  wiz5500_socket_enable_interrupt(wiz5500 *wiz, uint8_t socket, uint8_t ints);
uint8_t  wiz5500_socket_read_interrupt(wiz5500 *wiz, uint8_t socket);
uint8_t  wiz5500_socket_clear_interrupt(wiz5500 *wiz, uint8_t socket, uint8_t ints);

uint16_t wiz5500_get_socket_source_port(wiz5500 *wiz, uint8_t socket);
uint8_t  wiz5500_set_socket_dst_ip(wiz5500 *wiz, uint8_t socket, uint32_t ip);
uint8_t  wiz5500_set_socket_dst_port(wiz5500 *wiz, uint8_t socket, uint16_t port);


uint8_t  wiz5500_init(wiz5500 *wiz, SPI_HandleTypeDef *spi, GPIO_TypeDef *nss_port,
                      uint16_t nss_pin);
#endif
