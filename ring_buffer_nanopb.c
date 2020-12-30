#include <stdlib.h>
#include "ring_buffer_nanopb.h"

size_t pb_ring_buffer_metadata_used(struct ring_buffer_metadata* meta)
{
  // No rollover
  if (meta->head >= meta->tail) {
    return meta->head - meta->tail;
  }

  // Buffer rolled over
  return meta->size - meta->tail + meta->head;
}

size_t pb_ring_buffer_metadata_free(struct ring_buffer_metadata* meta)
{
  return meta->size - pb_ring_buffer_metadata_used(meta);
}


static bool pb_istream_read_callback(pb_istream_t *istream, uint8_t *buf, size_t count)
{
  struct ring_buffer_metadata* meta = istream->state;

  if (count == 0) {
    return true;
  }
  if (count > pb_ring_buffer_metadata_used(meta)) {
    // Not enough data in the buffer
    return false;
  }

  size_t tail_data_len = meta->size - meta->tail;

  // If requested data length does not rollover buffer so can be
  // done in single copy
  if (count <= tail_data_len) {
    memcpy(buf, &meta->buf[meta->tail], count);
  } else {
    // Copy data in 2 steps: remainder and the rest from the beginning
    memcpy(buf, &meta->buf[meta->tail], tail_data_len);
    buf += tail_data_len;
    memcpy(buf, meta->buf, count - tail_data_len);
  }

  // Update tail: we've read some data
  meta->tail = (meta->tail + count) % meta->size;

  return true;
}

static bool pb_ostream_write_callback(pb_ostream_t* ostream, const uint8_t* buf, size_t count)
{
  struct ring_buffer_metadata* meta = ostream->state;

  if (count == 0) {
    return true;
  }
  if (count > pb_ring_buffer_metadata_free(meta)) {
    // Not enough free space in the buffer
    return false;
  }

  size_t remainder = meta->size - meta->head;
  if (count <= remainder) {
    // Buffer is not rolled over, copy everything
    memcpy(&meta->buf[meta->head], buf, count);
  } else {
    // Rolled over buffer, 2 step data copy
    // Data fragment till the buffer end
    memcpy(&meta->buf[meta->head], buf, remainder);
    buf += remainder;
    // Second fragment is from beginning
    memcpy(meta->buf, buf, count - remainder);
  }
  // Update head: we've written some data
  meta->head = (meta->head + count) % meta->size;

  return true;
}

pb_istream_t pb_istream_from_ring_buffer(struct ring_buffer_metadata* meta, size_t msg_len)
{
  pb_istream_t stream;

  stream.bytes_left = msg_len;
  stream.callback = &pb_istream_read_callback;
  stream.state  = meta;

  return stream;
}

pb_ostream_t pb_ostream_from_ring_buffer(struct ring_buffer_metadata* meta)
{
  pb_ostream_t stream;

  stream.bytes_written = 0;
  stream.max_size = meta->size;
  stream.callback = &pb_ostream_write_callback;
  stream.state = meta;

  return stream;
}

void pb_ring_buffer_advance_tail(struct ring_buffer_metadata* meta, size_t len)
{
  meta->tail = (meta->tail + len) % meta->size;
}

void pb_ring_buffer_advance_head(struct ring_buffer_metadata* meta, size_t len)
{
  meta->head = (meta->head + len) % meta->size;
}
