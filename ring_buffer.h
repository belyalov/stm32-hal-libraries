// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#ifndef __RING_BUFFER_H
#define __RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
#define EXPORT extern "C"
#else
#define EXPORT
#endif

struct ring_buffer {
  uint8_t* buf;
  uint32_t tail;
  uint32_t head;
  uint32_t size;
  uint32_t used;
};

EXPORT void     ring_buffer_init(struct ring_buffer* meta, uint8_t* buf, uint32_t buf_size);

EXPORT bool     ring_buffer_write(struct ring_buffer* meta, uint8_t* buf, uint32_t write_size);
EXPORT bool     ring_buffer_read(struct ring_buffer* meta, uint8_t* buf, uint32_t read_size);

EXPORT void     ring_buffer_advance_head(struct ring_buffer* meta, uint32_t how_many);
EXPORT void     ring_buffer_reset(struct ring_buffer* meta);
EXPORT uint32_t ring_buffer_used(struct ring_buffer* meta);
EXPORT uint32_t ring_buffer_free(struct ring_buffer* meta);

#endif
