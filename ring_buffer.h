// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#ifndef __RING_BUFFER_H
#define __RING_BUFFER_H

// Declares ring buffer structure
#define RING_BUFFER_DECLARE(_name, _item, _size)      \
    struct _name##_ring {                             \
        _item buffer[_size];                          \
        size_t head;                                  \
        size_t tail;                                  \
        size_t size;                                  \
        uint8_t full;                                 \
    };                                                \
// Defines and initializes ring buffer structure.
// Params:
//  - _name: variable name to be created
//  - _item: ring buffer item type
//  - _size: ring size (items count)
#define RING_BUFFER_DEFINE(_name, _item, _size)              \
    struct _name##_ring _name##_ring_def = {{0}, 0, 0, _size}; \
    struct _name##_ring *_name = &_name##_ring_def;

// Pushes "_item" into ring buffer (does memcpy)
#define RING_BUFFER_PUSH_COPY(_name, _item)                                 \
    ({                                                                      \
        (_name)->head = ((_name)->head + 1) % (_name)->size;                \
        memcpy(&(_name)->buffer[(_name)->head], _item, sizeof(*_item));     \
        if (!(_name)->full) (_name)->full = (_name)->head == (_name)->tail; \
    })

// "Pushes" item (basically does pointer advance) and returns it.
// You must copy data yourself
#define RING_BUFFER_PUSH(_name)                                               \
    ({                                                                        \
        (_name)->head = ((_name)->head + 1) % (_name)->size;                  \
        if (!(_name)->full) (_name)->full = ((_name)->head == (_name)->tail); \
        &(_name)->buffer[(_name)->head];   \
    })

// Returns pointer to next element. Does NOT advance pointer
// You must copy data yourself
#define RING_BUFFER_GET_NEXT(_name) (&(_name)->buffer[(((_name)->head + 1) % (_name)->size)])

// POPs item from ring buffer (NOTE: it does not check for empty queue)
// and returns pointer to the item
#define RING_BUFFER_POP(_name)                                      \
    ({                                                              \
        (_name)->tail = ((_name)->tail + 1) % (_name)->size;        \
        (_name)->full = 0;                                          \
        &(_name)->buffer[(_name)->tail];                            \
    })

// Returns pointer to current tail
#define RING_BUFFER_TAIL(_name) (&(_name)->buffer[((_name)->tail + 1) % (_name)->size])

// Returns True if buffer is full
#define RING_BUFFER_FULL(_name) ((_name)->full)

// Returns True if buffer is empty
#define RING_BUFFER_EMPTY(_name) (!(_name)->full && (_name)->head == (_name)->tail)

#endif
