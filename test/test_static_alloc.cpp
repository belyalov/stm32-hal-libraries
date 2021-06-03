// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#include <gtest/gtest.h>
#include "static_alloc.h"

using namespace std;

extern "C" uint32_t unittest_is_block_used(uint32_t block);
extern "C" uint32_t unittest_is_block_free(uint32_t block);
extern "C" uint32_t unittest_blocks_count();
extern "C" uint8_t* unittest_user_data_starts_at();


TEST(static_alloc, sanity) {
  // Make buffer from initial 256 bytes.
  // Total available memory would be 256 - block size for metadata:
  // 256 - 64 = 192
  uint8_t buf[256];
  static_alloc_init(buf, sizeof(buf));

  // All free / correct number of blocks
  ASSERT_EQ(192, static_alloc_info_mem_free());
  ASSERT_EQ(3, unittest_blocks_count());
  // User data starts from the second block
  ASSERT_EQ(buf + STATIC_ALLOC_BLOCK_SIZE, unittest_user_data_starts_at());

  // Allocating more than buffer would fail
  void* too_big = static_alloc_alloc(1024);
  ASSERT_FALSE(too_big);

  // Allocate small item - 1 block will be used
  void* p1 = static_alloc_alloc(1);
  ASSERT_TRUE(p1);
  ASSERT_EQ(128, static_alloc_info_mem_free());
  ASSERT_TRUE(unittest_is_block_used(0));
  ASSERT_FALSE(unittest_is_block_used(1));

  // Allocate one more item of 2 block size, memory become full
  void* p2 = static_alloc_alloc(64);
  ASSERT_TRUE(p2);
  ASSERT_EQ(0, static_alloc_info_mem_free());
  ASSERT_TRUE(unittest_is_block_used(0));
  ASSERT_TRUE(unittest_is_block_used(1));
  ASSERT_TRUE(unittest_is_block_used(2));

  // Next alloc should fail (out of mem)
  void* p3 = static_alloc_alloc(1);
  ASSERT_FALSE(p3);
  ASSERT_EQ(0, static_alloc_info_mem_free());

  // Release big item (free 2 blocks)
  static_alloc_free(p2);
  ASSERT_EQ(128, static_alloc_info_mem_free());
  ASSERT_TRUE(unittest_is_block_used(0));
  ASSERT_FALSE(unittest_is_block_used(1));
  ASSERT_FALSE(unittest_is_block_used(2));

  // Try to allocate 2 small items on space freed by big item
  void* p4 = static_alloc_alloc(1);
  void* p5 = static_alloc_alloc(1);
  ASSERT_TRUE(p4);
  ASSERT_TRUE(p5);
  ASSERT_EQ(0, static_alloc_info_mem_free());

  // Release all items
  static_alloc_free(p1);
  static_alloc_free(p4);
  static_alloc_free(p5);

  // All memory is available again
  ASSERT_EQ(192, static_alloc_info_mem_free());
}

TEST(static_alloc, ref_counting) {
  // Make buffer for single block
  uint8_t buf[128];
  static_alloc_init(buf, sizeof(buf));

  // Allocate one item, then make a "copy"
  // Release item twice to ensure that first release does not mark memory as free
  void* p1 = static_alloc_alloc(1);
  ASSERT_TRUE(p1);
  ASSERT_EQ(0, static_alloc_info_mem_free());

  // "Copy" item
  void* p2 = static_alloc_copy(p1);
  ASSERT_EQ(p1, p2);

  // Release first item, memory should not be freed
  static_alloc_free(p1);
  ASSERT_EQ(0, static_alloc_info_mem_free());

  // Release second item
  static_alloc_free(p2);
  ASSERT_EQ(64, static_alloc_info_mem_free());
}

TEST(static_alloc, fragmentation1) {
  uint8_t buf[256];
  static_alloc_init(buf, sizeof(buf));

  // Allocate 2 items to fill all memory:
  // - single block len
  // - 2 blocks
  void* p1 = static_alloc_alloc(1);
  ASSERT_TRUE(p1);
  void* p2 = static_alloc_alloc(80);
  ASSERT_TRUE(p2);
  // 3 blocks total used, nothing free
  ASSERT_EQ(0, static_alloc_info_mem_free());

  // Release small item, try to allocate another big (2 blocks) item
  static_alloc_free(p1);
  ASSERT_EQ(64, static_alloc_info_mem_free());
  p1 = static_alloc_alloc(80);
  ASSERT_FALSE(p1);
  ASSERT_EQ(64, static_alloc_info_mem_free());

  // However, small item can still be allocated in last free block
  p1 = static_alloc_alloc(32);
  ASSERT_TRUE(p1);
  ASSERT_EQ(0, static_alloc_info_mem_free());
}

TEST(static_alloc, many_blocks) {
  uint8_t buf[4096];  // 64 blocks total minus 1 meta = 63
  static_alloc_init(buf, sizeof(buf));

  // Allocate big chunk of memory so next one will reside on 2 bitmasks
  void* p1 = static_alloc_alloc(30 * 64);
  ASSERT_TRUE(p1);
  ASSERT_EQ(2048, static_alloc_info_mem_free());

  // One more item
  void* p2 = static_alloc_alloc(10 * 64);
  ASSERT_TRUE(p2);
  ASSERT_EQ(1344, static_alloc_info_mem_free());

  // Free them all
  static_alloc_free(p2);
  static_alloc_free(p1);
  ASSERT_EQ(4032, static_alloc_info_mem_free());
}
