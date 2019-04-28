// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#include "bor.h"

uint8_t BOR_set_level(uint8_t level)
{
  FLASH_OBProgramInitTypeDef ob;
  uint32_t res = HAL_OK;

  // Update only if value differs from current
  // (it saves flash erase cycles)
  if (BOR_get_level() != level) {
    ob.BORLevel = level;
    ob.OptionType = OPTIONBYTE_BOR;
    // Write into flash
    HAL_FLASH_OB_Unlock();
    HAL_FLASHEx_OBProgram(&ob);
    // Launch new value
    res = HAL_FLASH_OB_Launch();
    // Lock flash
    HAL_FLASH_OB_Lock();
  }
  return res;
}

uint8_t BOR_get_level()
{
  // Read all options
  FLASH_OBProgramInitTypeDef ob;
  HAL_FLASHEx_OBGetConfig(&ob);

  return ob.BORLevel;
}

