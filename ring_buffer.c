// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#include <string.h>
#include <stdio.h>
#include "ring_buffer.h"


void ring_buffer_init(struct ring_buffer* meta, uint8_t* buf, uint32_t buf_size)
{
  meta->buf = buf;
  meta->head = 0;
  meta->tail = 0;
  meta->used = 0;
  meta->size = buf_size;
}

bool ring_buffer_write(struct ring_buffer* meta, uint8_t* buf, uint32_t write_size)
{
  if (write_size == 0) {
    return true;
  }
  if (write_size > ring_buffer_free(meta)) {
    // Not enough free space in the buffer
    return false;
  }

  uint32_t remainder = meta->size - meta->head;
  if (write_size <= remainder) {
    // Buffer is not rolled over, copy everything
    memcpy(&meta->buf[meta->head], buf, write_size);
  } else {
    // Rolled over buffer, 2 step data copy
    // Data fragment till the buffer end
    memcpy(&meta->buf[meta->head], buf, remainder);
    buf += remainder;
    // Second fragment is from beginning
    memcpy(meta->buf, buf, write_size - remainder);
  }
  // Update head/size: we've written some data
  meta->head = (meta->head + write_size) % meta->size;
  meta->used += write_size;

  return true;
}

bool ring_buffer_read(struct ring_buffer* meta, uint8_t* buf, uint32_t read_size)
{
  if (read_size == 0) {
    return true;
  }
  if (read_size > meta->used) {
    // Not enough data in the buffer
    return false;
  }

  uint32_t tail_data_len = meta->size - meta->tail;

  // If requested data length does not rollover buffer so can be
  // done in single copy
  if (read_size <= tail_data_len) {
    memcpy(buf, &meta->buf[meta->tail], read_size);
  } else {
    // Copy data in 2 steps: remainder and the rest from the beginning
    memcpy(buf, &meta->buf[meta->tail], tail_data_len);
    buf += tail_data_len;
    memcpy(buf, meta->buf, read_size - tail_data_len);
  }

  // Update tail/used: we've read some data
  meta->tail = (meta->tail + read_size) % meta->size;
  meta->used -= read_size;

  return true;
}

uint32_t ring_buffer_used(struct ring_buffer* meta)
{
  return meta->used;
}

uint32_t ring_buffer_free(struct ring_buffer* meta)
{
  return meta->size - meta->used;
}

void ring_buffer_advance_head(struct ring_buffer* meta, uint32_t how_many)
{
  meta->used += how_many;
  meta->head = (meta->head + how_many) % meta->size;
}

void ring_buffer_reset(struct ring_buffer* meta)
{
  meta->head = 0;
  meta->tail = 0;
  meta->used = 0;
}
