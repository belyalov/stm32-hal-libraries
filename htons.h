// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#ifndef HTONS_H
#define HTONS_H

#define HTONS(x) ( ((x) << 8) | ( ((x) >> 8) & 0xFF) )
#define NTOHS(x) HTONS(x)

#define HTONL(x) ( ((x) << 24 & 0xFF000000UL) |         \
                   ((x) <<  8 & 0x00FF0000UL) |         \
                   ((x) >>  8 & 0x0000FF00UL) |         \
                   ((x) >> 24 & 0x000000FFUL) )
#define NTOHL(x) HTONL(x)

#endif

