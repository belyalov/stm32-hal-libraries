// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#include "lora_pb.h"

#include "pb_encode.h"
#include "pb_decode.h"

#define DEVICE_ID_SIZE            sizeof(uint64_t)
#define AES_BLOCK_SIZE            16
#define MAX_PAYLOAD_SIZE          (LORA_MAX_PACKET_SIZE - DEVICE_ID_SIZE - AES_BLOCK_SIZE)

// // Define continuous buffer
// // Buffer must be aligned to u32 in order to make hardware AES work
__ALIGN_BEGIN
static uint8_t    _buffer[LORA_MAX_PACKET_SIZE * 2]
__ALIGN_END;

// DEV_ID -- AES_IV -- ENCRYPTED_MESSAGE -- SERIALIZED_MESSAGE
// (0--------------------------------128)...(128-----------256)
static uint64_t   *buf_device_id    = (uint64_t*) _buffer;
static uint8_t    *buf_aes_iv       = &_buffer[DEVICE_ID_SIZE];
static uint8_t    *buf_aes_encrypt  = &_buffer[DEVICE_ID_SIZE + AES_BLOCK_SIZE];
static uint8_t    *buf_pb_serialize = &_buffer[LORA_MAX_PACKET_SIZE];

// AES hardware module
extern CRYP_HandleTypeDef hcryp;


uint32_t lora_pb_encode_and_send_blocking(lora_sx1276 *lora,
                                          uint8_t *aes_key, const uint64_t device_id,
                                          const pb_msgdesc_t *pb_fields, const void *pb_struct)
{
  // Copy device id
  *buf_device_id = device_id;

  // Serialize protobuf
  pb_ostream_t stream = pb_ostream_from_buffer(buf_pb_serialize, LORA_MAX_PACKET_SIZE);
  if (!pb_encode(&stream, pb_fields, pb_struct)) {
    return LORA_PB_SERIALIZE_FAILED;
  }

  if (stream.bytes_written > MAX_PAYLOAD_SIZE) {
    return LORA_PB_TOO_LONG;
  }

  // AES initialization vector:
  // First byte contains actual payload len, the rest is just junk values
  buf_aes_iv[0] = stream.bytes_written;
  for (int i = 1; i < AES_BLOCK_SIZE; i++) {
    buf_aes_iv[i] = HAL_GetTick() % (254 - i);
  }
  // Add padding, if needed
  uint32_t total_encoded_len = stream.bytes_written;
  if ((total_encoded_len % AES_BLOCK_SIZE) != 0) {
      total_encoded_len += AES_BLOCK_SIZE - total_encoded_len % AES_BLOCK_SIZE;
  }
  // Init AES module
  hcryp.Instance = AES;
  hcryp.Init.DataType = CRYP_DATATYPE_8B;
  hcryp.Init.pKey = aes_key;
  hcryp.Init.pInitVect = buf_aes_iv;
  int res = HAL_CRYP_Init(&hcryp);
  if (res != HAL_OK) {
    return LORA_PB_AES_INIT_FAILED;
  }

  // Perform AES encryption
  res = HAL_CRYP_AESCBC_Encrypt(&hcryp, buf_pb_serialize, total_encoded_len,
      buf_aes_encrypt, LORA_PB_TIMEOUT);
  HAL_CRYP_DeInit(&hcryp);
  if (res != HAL_OK) {
    return LORA_PB_ENCODE_FAILED;
  }

  // Send message
  uint32_t total_msg_len = DEVICE_ID_SIZE + AES_BLOCK_SIZE + total_encoded_len;
  return lora_send_packet_blocking(lora, _buffer, total_msg_len, LORA_PB_TIMEOUT);
}
