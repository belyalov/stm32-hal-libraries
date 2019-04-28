// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#ifndef __BOR_H
#define __BOR_H

#include "main.h"

#define BOR_OFF           OB_BOR_OFF
#define BOR_LEVEL_1_7V    OB_BOR_LEVEL1
#define BOR_LEVEL_1_9V    OB_BOR_LEVEL2
#define BOR_LEVEL_2_3V    OB_BOR_LEVEL3
#define BOR_LEVEL_2_55V   OB_BOR_LEVEL4
#define BOR_LEVEL_2_8V    OB_BOR_LEVEL5


uint8_t BOR_set_level(uint8_t level);
uint8_t BOR_get_level();


#endif