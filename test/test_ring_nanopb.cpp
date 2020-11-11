// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#include <gtest/gtest.h>

#include "ring_buffer_nanopb.h"
#include "pb_decode.h"

using namespace std;

TEST(ring_buffer_nanopb, setters_getters) {
  struct ring_buffer_metadata meta = {};
  meta.size = 10;

  // Emulate that buffer just got 2 bytes
  // **--------
  // T H
  meta.tail = 0;
  meta.head = 2;
  ASSERT_EQ(ring_buffer_metadata_get_length(&meta), 2);

  // "Read" 2 bytes
  meta.tail = 2;
  ASSERT_EQ(ring_buffer_metadata_get_length(&meta), 0);

  // Right edge: last 8 bytes used, head rolled over
  // --********
  // H T
  meta.head = 0;
  ASSERT_EQ(ring_buffer_metadata_get_length(&meta), 8);

  // Rolled over buffer:
  // **--******
  //   H T
  meta.tail = 4;
  meta.head = 2;
  ASSERT_EQ(ring_buffer_metadata_get_length(&meta), 8);
}

TEST(ring_buffer_nanopb, read_callback)
{
  // Init some test buffer
  uint8_t buf[10];
  for (size_t i = 0; i < sizeof(buf); i++) {
    buf[i] = i;
  }

  struct ring_buffer_metadata meta = {};
  meta.buf = buf;
  meta.size = sizeof(buf);
  pb_istream_t stream = pb_istream_from_ring_buffer(&meta);

  // "Put" some data into buffer
  meta.head = 6;

  // "Read" 5 bytes of 6 total in buffer
  {
    uint8_t test[5];
    bool res = stream.callback(&stream, test, sizeof(test));
    // Verify data
    ASSERT_TRUE(res);
    for (size_t i = 0; i < sizeof(test); i++) {
      ASSERT_EQ(test[i], i) << "index " << i;
    }
  }
  // "Read" last byte
  {
    uint8_t test;
    bool res = stream.callback(&stream, &test, 1);
    ASSERT_TRUE(res);
    ASSERT_EQ(test, 5);
  }
  // Ensure that callback will fail when there is no more data
  ASSERT_FALSE(stream.callback(&stream, NULL, 1));

  // Buffer rollover use case (8 bytes: 6 by the end and 2 from the beginning)
  // **--******
  meta.tail = 4;
  meta.head = 2;

  uint8_t test[8];
  bool res = stream.callback(&stream, test, sizeof(test));
  // Verify data
  ASSERT_TRUE(res);
  for (size_t i = 0; i < sizeof(test); i++) {
    ASSERT_EQ(test[i], (i + 4) % 10);
  }
}

// TEST(ring_buffer_nanopb, encode_decode)
// {

// }
