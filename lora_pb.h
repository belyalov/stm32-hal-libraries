// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#ifndef __LORA_PB_H
#define __LORA_PB_H

#include "main.h"
#include "lora_sx1276.h"
#include "pb.h"

// LoRa PB errors
#define LORA_PB_OK                           LORA_OK
#define LORA_PB_SERIALIZE_FAILED             32
#define LORA_PB_ENCODE_FAILED                33
#define LORA_PB_TOO_LONG                     34
#define LORA_PB_AES_INIT_FAILED              35

// Timeout for blocking calls: AES encrypt/decrypt / LoRa send
#ifndef LORA_PB_TIMEOUT
#define LORA_PB_TIMEOUT                     1000
#endif

// NanoPB and Lora helper: 
// -> Serialize given protobuf
// -> Encode it using hardware AES CBC
// -> Send wirelessly using pre-setup lora
//
// Packet structure will look like
// [0-7: device_id] [8-24: AES_IV] [25-...: Serialized protobuf]
uint32_t lora_pb_encode_and_send_blocking(lora_sx1276 *lora,
                                          uint8_t *aes_key, const uint64_t device_id,
                                          const pb_msgdesc_t *fields, const void *pb);

#endif
