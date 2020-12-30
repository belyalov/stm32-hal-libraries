// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#include <gtest/gtest.h>
#include "ring_buffer.h"

using namespace std;


TEST(ring_buffer, read_write)
{
  struct ring_buffer ring;
  uint8_t ringbuf[10] = {};
  ring_buffer_init(&ring, ringbuf, sizeof(ringbuf));

  // Simulate that some data has written externally (advance)
  {
    ring_buffer_advance_head(&ring, 3);
    ASSERT_EQ(3, ring_buffer_used(&ring));
    uint8_t tmp[10] = {};
    bool res = ring_buffer_read(&ring, tmp, 3);
    ASSERT_TRUE(res);
    for (size_t i = 0; i < 3; i++) {
      ASSERT_EQ(tmp[i], 0);
    }
  }

  // Read from empty buffer
  {
    uint8_t tmp[10] = {};
    bool res = ring_buffer_read(&ring, tmp, 1);
    ASSERT_FALSE(res);
  }

  // Write too big chunk
  bool res = ring_buffer_write(&ring, NULL, 20);
  ASSERT_FALSE(res);

  // Write/Read chunk
  {
    uint8_t wr[10] = {1,2,3,4};
    bool res = ring_buffer_write(&ring, wr, 4);
    ASSERT_TRUE(res);
    ASSERT_EQ(4, ring_buffer_used(&ring));
    uint8_t rd[10] = {};
    res = ring_buffer_read(&ring, rd, 4);
    ASSERT_TRUE(res);
    // Verify data
    for (size_t i = 0; i < 4; i++) {
      ASSERT_EQ(wr[i], rd[i]) << "index " << i;
    }
  }

  // Write/Read chunk: rollover
  {
    uint8_t wr[10] = {1,2,3,4, 5, 6, 7};
    bool res = ring_buffer_write(&ring, wr, 7);
    ASSERT_TRUE(res);
    ASSERT_EQ(7, ring_buffer_used(&ring));

    uint8_t rd[10] = {};
    res = ring_buffer_read(&ring, rd, 7);
    ASSERT_TRUE(res);
    // Verify data
    for (size_t i = 0; i < 7; i++) {
      ASSERT_EQ(wr[i], rd[i]) << "index " << i;
    }
  }

  // Write/Read full buffer at once (will also roll over buffer)
  {
    uint8_t wr[10] = {1,2,3,4,5,6,7,8,9,10};
    bool res = ring_buffer_write(&ring, wr, sizeof(wr));
    ASSERT_TRUE(res);
    ASSERT_EQ(10, ring_buffer_used(&ring));

    uint8_t rd[10] = {};
    res = ring_buffer_read(&ring, rd, sizeof(wr));
    ASSERT_TRUE(res);
    ASSERT_EQ(0, ring_buffer_used(&ring));
    // Verify data
    for (size_t i = 0; i < sizeof(wr); i++) {
      ASSERT_EQ(wr[i], rd[i]) << "index " << i;
    }
  }

  // Write x3 / Read x3 chunks
  {
    uint8_t wr[10] = {0,1,2,3,4};
    for (size_t i = 0; i < 3; i++) {
      bool res = ring_buffer_write(&ring, wr, 3);
      ASSERT_TRUE(res);
    }
    ASSERT_EQ(9, ring_buffer_used(&ring));

    uint8_t rd[10] = {};
    for (size_t i = 0; i < 3; i++) {
      res = ring_buffer_read(&ring, rd, 3);
      ASSERT_TRUE(res);
      // Verify data
      for (size_t j = 0; j < 3; j++) {
        ASSERT_EQ(rd[j], j) << "index " << j;
      }
    }
  }
}

TEST(ring_buffer, reset)
{
  struct ring_buffer ring;
  uint8_t ringbuf[10] = {};
  ring_buffer_init(&ring, ringbuf, sizeof(ringbuf));

  // "Write" some data
  ring_buffer_advance_head(&ring, 5);
  ASSERT_EQ(5, ring_buffer_used(&ring));

  // Reset buffer
  ring_buffer_reset(&ring);
  ASSERT_EQ(0, ring_buffer_used(&ring));
}
