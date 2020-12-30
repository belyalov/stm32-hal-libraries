// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#ifndef __RING_BUFFER_NANOPB_H
#define __RING_BUFFER_NANOPB_H

#include <pb_encode.h>
#include <pb_decode.h>

#ifdef __cplusplus
#define EXPORT extern "C"
#else
#define EXPORT
#endif

struct ring_buffer_metadata {
  uint8_t* buf;
  uint32_t tail;
  uint32_t head;
  uint32_t size;
};

EXPORT pb_ostream_t pb_ostream_from_ring_buffer(struct ring_buffer_metadata* meta);
EXPORT pb_istream_t pb_istream_from_ring_buffer(struct ring_buffer_metadata* meta, size_t msglen);
EXPORT size_t       pb_ring_buffer_metadata_used(struct ring_buffer_metadata* meta);
EXPORT size_t       pb_ring_buffer_metadata_free(struct ring_buffer_metadata* meta);
EXPORT void         pb_ring_buffer_advance_tail(struct ring_buffer_metadata* meta, size_t len);
EXPORT void         pb_ring_buffer_advance_head(struct ring_buffer_metadata* meta, size_t len);

#endif
