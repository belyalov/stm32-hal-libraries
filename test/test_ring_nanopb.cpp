// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#include <gtest/gtest.h>

#include "ring_buffer_nanopb.h"
#include "pb_encode.h"
#include "pb_decode.h"

#include "proto/sample.pb.h"

using namespace std;

TEST(ring_buffer_nanopb, setters_getters) {
  struct ring_buffer_metadata meta = {};
  meta.size = 10;

  // No data
  meta.tail = 5;
  meta.head = 5;
  ASSERT_EQ(pb_ring_buffer_metadata_used(&meta), 0);
  ASSERT_EQ(pb_ring_buffer_metadata_free(&meta), 10);

  // Emulate that buffer just got 2 bytes
  // **--------
  // T H
  meta.tail = 0;
  meta.head = 2;
  ASSERT_EQ(pb_ring_buffer_metadata_used(&meta), 2);
  ASSERT_EQ(pb_ring_buffer_metadata_free(&meta), 8);

  // "Read" 2 bytes
  meta.tail = 2;
  ASSERT_EQ(pb_ring_buffer_metadata_used(&meta), 0);
  ASSERT_EQ(pb_ring_buffer_metadata_free(&meta), 10);

  // Right edge: last 8 bytes used, head rolled over
  // --********
  // H T
  meta.head = 0;
  ASSERT_EQ(pb_ring_buffer_metadata_used(&meta), 8);
  ASSERT_EQ(pb_ring_buffer_metadata_free(&meta), 2);

  // Rolled over buffer:
  // **--******
  //   H T
  meta.tail = 4;
  meta.head = 2;
  ASSERT_EQ(pb_ring_buffer_metadata_used(&meta), 8);
  ASSERT_EQ(pb_ring_buffer_metadata_free(&meta), 2);

  // Advance tail
  meta.tail = 0;
  meta.head = 0;
  pb_ring_buffer_advance_tail(&meta, 5);
  ASSERT_EQ(meta.tail, 5);
  pb_ring_buffer_advance_tail(&meta, 5);
  ASSERT_EQ(meta.tail, 0);

  // Advance head
  meta.tail = 0;
  meta.head = 0;
  pb_ring_buffer_advance_head(&meta, 5);
  ASSERT_EQ(meta.head, 5);
  pb_ring_buffer_advance_head(&meta, 5);
  ASSERT_EQ(meta.head, 0);
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
  pb_istream_t stream = pb_istream_from_ring_buffer(&meta, 0);

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

TEST(ring_buffer_nanopb, write_callback)
{
  uint8_t buf[6] = {};
  struct ring_buffer_metadata meta = {};
  meta.buf = buf;
  meta.size = sizeof(buf);
  pb_ostream_t stream = pb_ostream_from_ring_buffer(&meta);

  // write some data
  uint8_t tmp[4] = {1, 2, 3, 4};
  stream.callback(&stream, tmp, sizeof(tmp));
  // verify that it has been written well
  ASSERT_EQ(pb_ring_buffer_metadata_used(&meta), 4);
  for (size_t i = 0; i < sizeof(tmp); i++) {
    ASSERT_EQ(buf[i], tmp[i]);
  }

  // "read" it
  meta.tail = meta.head;

  // write it again: this will cause buffer to rollover
  stream.callback(&stream, tmp, sizeof(tmp));
  ASSERT_EQ(pb_ring_buffer_metadata_used(&meta), 4);
  // verity it one more time: part of data should be rolled over
  ASSERT_EQ(buf[4], tmp[0]);
  ASSERT_EQ(buf[5], tmp[1]);
  ASSERT_EQ(buf[0], tmp[2]);
  ASSERT_EQ(buf[1], tmp[3]);
}

TEST(ring_buffer_nanopb, encode_decode)
{
  uint8_t buf[32];
  struct ring_buffer_metadata meta = {};
  meta.buf = buf;
  meta.size = sizeof(buf);

  protobufs_Test msg = protobufs_Test_init_default;
  msg.foo = 1;
  msg.bar = 2;
  msg.baz = true;
  msg.has_sub = true;
  msg.sub.sub_foo = 3;
  msg.sub.sub_bar = 4;

  // Do encode/decode message using ring buffer multiple times
  // Since buffer size a bit larger that encoded message size
  // it will effectively trigger buffer roll over from time to time
  for (size_t i = 0; i < 20; i++) {
    // Encode message into ring buffer
    pb_ostream_t ostream = pb_ostream_from_ring_buffer(&meta);
    bool res = pb_encode(&ostream, protobufs_Test_fields, &msg);
    ASSERT_TRUE(res);

    // Then decode it back
    protobufs_Test msg2 = protobufs_Test_init_default;
    pb_istream_t istream = pb_istream_from_ring_buffer(&meta, ostream.bytes_written);

    res = pb_decode(&istream, protobufs_Test_fields, &msg2);
    ASSERT_TRUE(res);

    // Verify that messages match
    ASSERT_EQ(msg.foo, msg2.foo);
    ASSERT_EQ(msg.bar, msg2.bar);
    ASSERT_EQ(msg.baz, msg2.baz);
    ASSERT_EQ(msg.has_sub, msg2.has_sub);
    ASSERT_EQ(msg.sub.sub_foo, msg2.sub.sub_foo);
    ASSERT_EQ(msg.sub.sub_bar, msg2.sub.sub_bar);
  }
}
