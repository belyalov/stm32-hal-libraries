#include <stdlib.h>
#include "ring_buffer_nanopb.h"

size_t ring_buffer_metadata_get_length(struct ring_buffer_metadata* meta)
{
  // No rollover
  if (meta->head >= meta->tail) {
    return meta->head - meta->tail;
  }

  // Buffer rolled over
  return meta->size - meta->tail + meta->head;
}


static bool pb_istream_read_callback(pb_istream_t *istream, uint8_t *buf, size_t count)
{
  struct ring_buffer_metadata* meta = istream->state;

  if (count == 0) {
    return true;
  }
  if (count > ring_buffer_metadata_get_length(meta)) {
    // Not enough data in the buffer
    return false;
  }

  if (meta->head > meta->tail) {
    // Buffer is not rolled over
    memcpy(buf, &meta->buf[meta->tail], count);
  } else {
    // Rolled over buffer, 2 step data copy
    // Data fragment till the buffer end
    size_t tail_data_len = meta->size - meta->tail;
    memcpy(buf, &meta->buf[meta->tail], tail_data_len);
    buf += tail_data_len;
    // Second fragment is from beginning
    memcpy(buf, meta->buf, count - tail_data_len);
  }
  // Update tail: we've read some data
  meta->tail = (meta->tail + count) % meta->size;

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
