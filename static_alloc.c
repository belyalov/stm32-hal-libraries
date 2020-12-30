// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.
#include <string.h>
#include <stdio.h>

#include "static_alloc.h"

// Use mutex for alloc/free operation under freertos
#ifdef STATIC_ALLOC_FREERTOS
#include "FreeRTOS.h"
#include "semphr.h"
static SemaphoreHandle_t  _mutex;
static StaticSemaphore_t  _mutex_buffer;
#endif


struct static_alloc_item {
  uint16_t refcount;
  uint16_t blocks_used;
};

static uint32_t* _blocks_status;
static uint8_t*  _blocks_user_data;
static uint32_t  _blocks_count;

#define MARK_BLOCK_AS_USED(n)     do { _blocks_status[n / 32] &= ~(1 << (n & 31)); } while(0)
#define MARK_BLOCK_AS_FREE(n)     do { _blocks_status[n / 32] |= 1 << (n & 31); } while(0)
#define IS_BLOCK_FREE(n)          (_blocks_status[n / 32] & (1 << (n & 31)))


void static_alloc_init(uint8_t* buf, uint32_t buf_size)
{
  memset(buf, 0, buf_size);
  _blocks_status = (uint32_t*)buf;
  // First block reserved for metadata
  _blocks_count = (buf_size / STATIC_ALLOC_BLOCK_SIZE) - 1;
  // User blocks located right after block statuses
  _blocks_user_data = buf + (((_blocks_count + 31) / 32) * 4);
  // Init block status: all free
  for (uint32_t i = 0; i < _blocks_count; i++) {
    MARK_BLOCK_AS_FREE(i);
  }
  // Create mutex if RTOS enabled
#ifdef STATIC_ALLOC_FREERTOS
  _mutex = xSemaphoreCreateMutexStatic(&_mutex_buffer);
#endif
}

void* static_alloc_alloc(uint32_t size)
{
  uint32_t total_size = size + sizeof(struct static_alloc_item);
  uint32_t blocks_required = (total_size + STATIC_ALLOC_BLOCK_SIZE - 1) / STATIC_ALLOC_BLOCK_SIZE;
  uint32_t blocks_found = 0;
  void *   result = NULL;

  // FreeRTOS requires critical section in order to be task safe
#ifdef STATIC_ALLOC_FREERTOS
  if (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE) {
    return NULL;
  }
#endif
  // Find first free block
  // TODO: this could be improved by using __buildin_* functions to
  // count bits, however it will complicate the code.
  for (uint32_t i = 0; i < _blocks_count; i++) {
    if (IS_BLOCK_FREE(i)) {
      blocks_found++;
    } else {
      blocks_found = 0;
    }
    if (blocks_found == blocks_required) {
      uint32_t first_block = (i - blocks_required + 1);
      uint32_t offset = first_block * STATIC_ALLOC_BLOCK_SIZE;
      // Mark all these blocks as used
      for (uint32_t j = first_block; j <= i; j++) {
        MARK_BLOCK_AS_USED(j);
      }
      // Setup metadata and return pointer next to metadata
      struct static_alloc_item* item = (struct static_alloc_item*)(_blocks_user_data + offset);
      item->blocks_used = blocks_required;
      item->refcount = 1;
      result = ((uint8_t*)item + sizeof(struct static_alloc_item));
      break;
    }
  }
  // Out of memory: unable to find continuos array of N blocks

end:
  // Release mutex for RTOS version
#ifdef STATIC_ALLOC_FREERTOS
  xSemaphoreGive(_mutex);
#endif
  return result;
}

void* static_alloc_copy(void* ptr)
{
  struct static_alloc_item* item = (struct static_alloc_item*)((uint8_t*)ptr - sizeof(struct static_alloc_item));
  item->refcount++;

  return ptr;
}

void static_alloc_free(void* ptr)
{
  struct static_alloc_item* item = (struct static_alloc_item*)((uint8_t*)ptr - sizeof(struct static_alloc_item));

  // FreeRTOS requires critical section in order to be task safe
#ifdef STATIC_ALLOC_FREERTOS
  if (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE) {
    return NULL;
  }
#endif

  // Dec ref count
  if (item->refcount > 0) {
    item->refcount--;
  }
  // If nobody else using this memory - mark it as free
  if (item->refcount == 0) {
    // Find first block index
    uint32_t index = ((uint8_t*)item - _blocks_user_data) / STATIC_ALLOC_BLOCK_SIZE;
    // Mark all allocated blocks as free
    for (uint32_t i = index; i < index + item->blocks_used; i++) {
      MARK_BLOCK_AS_FREE(i);
    }
  }

  // Release mutex for RTOS version
#ifdef STATIC_ALLOC_FREERTOS
  xSemaphoreGive(_mutex);
#endif
}

uint32_t static_alloc_info_mem_free(void)
{
  uint32_t free = 0;

  for (uint32_t i = 0; i < _blocks_count; i++) {
    if (IS_BLOCK_FREE(i)) {
      free += STATIC_ALLOC_BLOCK_SIZE;
    }
  }
  return free;
}
