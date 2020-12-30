// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#include <gtest/gtest.h>

#include "ring_buffer_fixed_size.h"

using namespace std;

struct item {
  uint32_t a, b;
};


TEST(ring_buffer_fixed_size, push_pop_simple)
{
  // Create ring buffer of 10 items
  RING_BUFFER_DECLARE(rbuf, struct item, 10);
  RING_BUFFER_DEFINE(rbuf, struct item, 10);
  // Ensure that it is empty :)
  ASSERT_TRUE(RING_BUFFER_EMPTY(rbuf));
  // Size initialized correctly
  ASSERT_EQ(rbuf->size, 10);

  // Push item using PUSH_COPY
  struct item item1 = {33, 44};
  RING_BUFFER_PUSH_COPY(rbuf, &item1);
  ASSERT_FALSE(RING_BUFFER_EMPTY(rbuf));
  ASSERT_FALSE(RING_BUFFER_FULL(rbuf));

  // Pop item
  struct item *item2 = RING_BUFFER_TAIL(rbuf);
  ASSERT_EQ(33, item2->a);
  ASSERT_EQ(44, item2->b);
  // RING_BUFFER_TAIL does not move pointer
  ASSERT_FALSE(RING_BUFFER_EMPTY(rbuf));
  ASSERT_FALSE(RING_BUFFER_FULL(rbuf));
  struct item *item3 = RING_BUFFER_POP(rbuf);
  ASSERT_EQ(item2, item3);
  ASSERT_TRUE(RING_BUFFER_EMPTY(rbuf));
  ASSERT_FALSE(RING_BUFFER_FULL(rbuf));
}

TEST(ring_buffer_fixed_size, push_pop_all)
{
  // Create ring buffer of 10 items
  RING_BUFFER_DECLARE(rbuf, struct item, 10);
  RING_BUFFER_DEFINE(rbuf, struct item, 10);
  // Ensure that it is empty :)
  ASSERT_TRUE(RING_BUFFER_EMPTY(rbuf));

  // Add item 10 items
  for (uint32_t i = 0; i < 9; i++) {
    struct item i_pushed = {i * 10, i * 100};
    RING_BUFFER_PUSH_COPY(rbuf, &i_pushed);
    ASSERT_FALSE(RING_BUFFER_FULL(rbuf));
    ASSERT_FALSE(RING_BUFFER_EMPTY(rbuf));
  }
  // Edge item
  {
    struct item *pushed = RING_BUFFER_PUSH(rbuf);
    ASSERT_TRUE(RING_BUFFER_FULL(rbuf));
    ASSERT_FALSE(RING_BUFFER_EMPTY(rbuf));
    pushed->a = 90;
    pushed->b = 900;
  }
  // Pop all 10 items
  for (uint32_t i = 0; i < 10; i++) {
    ASSERT_FALSE(RING_BUFFER_EMPTY(rbuf));
    struct item *i_pop = RING_BUFFER_POP(rbuf);
    ASSERT_EQ(i * 10, i_pop->a);
    ASSERT_EQ(i * 100, i_pop->b);
    ASSERT_FALSE(RING_BUFFER_FULL(rbuf));
  }
  ASSERT_TRUE(RING_BUFFER_EMPTY(rbuf));
}

TEST(ring_buffer_fixed_size, push_override)
{
  // Create ring buffer of 10 items
  RING_BUFFER_DECLARE(rbuf, struct item, 10);
  RING_BUFFER_DEFINE(rbuf, struct item, 10);

  // Add item 20 items (first 10 must be overridden)
  for (uint32_t i = 0; i < 20; i++) {
    struct item i_pushed = {i * 10, i * 100};
    RING_BUFFER_PUSH_COPY(rbuf, &i_pushed);
    if (i >= 9) {
      ASSERT_TRUE(RING_BUFFER_FULL(rbuf));
    } else {
      ASSERT_FALSE(RING_BUFFER_FULL(rbuf));
    }
  }

  // Pop all items (expect only last 10)
  for (uint32_t i = 0; i < 10; i++) {
    struct item *i_pop = RING_BUFFER_POP(rbuf);
    ASSERT_EQ((i + 10) * 10, i_pop->a);
    ASSERT_EQ((i + 10) * 100, i_pop->b);
    ASSERT_FALSE(RING_BUFFER_FULL(rbuf));
  }
}
