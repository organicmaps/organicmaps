#pragma once

#include "indexer/scales.hpp"

#include "base/assert.hpp"
#include "base/macros.hpp"

#include <cstring>
#include <string>

namespace strings { class UniString; }

namespace feature
{
static int const g_arrWorldScales[] = {3, 5, 7, 9};
static_assert(9 == scales::GetUpperWorldScale(), "");
static int const g_arrCountryScales[] = {10, 12, 14, 17};
static_assert(17 == scales::GetUpperScale(), "");

inline std::string GetTagForIndex(std::string const & prefix, size_t ind)
{
  static char const arrChar[] = {'0', '1', '2', '3'};
  static_assert(ARRAY_SIZE(arrChar) >= ARRAY_SIZE(g_arrWorldScales), "");
  static_assert(ARRAY_SIZE(arrChar) >= ARRAY_SIZE(g_arrCountryScales), "");
  ASSERT(ind >= 0 && ind < ARRAY_SIZE(arrChar), (ind));

  return prefix + arrChar[ind];
}

bool IsDigit(int c);
bool IsNumber(strings::UniString const & s);

bool IsHouseNumber(std::string const & s);
bool IsHouseNumber(strings::UniString const & s);
bool IsHouseNumberDeepCheck(strings::UniString const & s);

uint8_t PopulationToRank(uint64_t p);
uint64_t RankToPopulation(uint8_t r);
} // namespace feature
