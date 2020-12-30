// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#ifndef STATIC_ALLOCATOR_H
#define STATIC_ALLOCATOR_H

#include <stdint.h>

#ifndef STATIC_ALLOC_BLOCK_SIZE
#define STATIC_ALLOC_BLOCK_SIZE      64
#endif

#ifdef __cplusplus
#define EXPORT extern "C"
#else
#define EXPORT
#endif


EXPORT void  static_alloc_init(uint8_t* buf, uint32_t buf_size);
EXPORT void* static_alloc_alloc(uint32_t size);
EXPORT void* static_alloc_copy(void* ptr);
EXPORT void  static_alloc_free(void* ptr);

EXPORT uint32_t static_alloc_info_mem_free(void);

#endif
