// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.

#include <gtest/gtest.h>

// Under several systems macros HTON* are already defined.
// Undef them before.
#undef HTONS
#undef NTOHS
#undef HTONL
#undef NTOHL
#include "htons.h"

using namespace std;

TEST(utils, htons)
{
  map<uint16_t, uint16_t> runs = {
    {0, 0},
    {1, 0x0100},
    {0x0100, 1},
    {0x2233, 0x3322},
    {0xFFFF, 0xFFFF},
  };

  for (auto it = runs.begin(); it != runs.end(); ++it) {
    uint16_t res = HTONS(it->first);
    // Check result
    ASSERT_EQ(it->second, res);
  }
}

TEST(utils, htonl)
{
  map<uint32_t, uint32_t> runs = {
    {0, 0},
    {1, 0x01000000},
    {0x01000000, 1},
    {0x22334455, 0x55443322},
    {0xFFFFFFFF, 0xFFFFFFFF},
  };

  for (auto it = runs.begin(); it != runs.end(); ++it) {
    uint32_t res = HTONL(it->first);
    // Check result
    ASSERT_EQ(it->second, res);
  }
}
