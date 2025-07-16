#pragma once

#include "indexer/scales.hpp"

#include "base/assert.hpp"

#include <string>

namespace strings
{
class UniString;
}

namespace feature
{
int constexpr g_arrWorldScales[] = {3, 5, 7, scales::GetUpperWorldScale()};
int constexpr g_arrCountryScales[] = {10, 12, 14, scales::GetUpperScale()};
static_assert(std::size(g_arrWorldScales) == std::size(g_arrCountryScales));

inline std::string GetTagForIndex(std::string const & prefix, size_t ind)
{
  ASSERT(ind < std::size(g_arrWorldScales) && ind < std::size(g_arrCountryScales), (ind));
  return prefix + char('0' + ind);
}

uint8_t PopulationToRank(uint64_t p);
uint64_t RankToPopulation(uint8_t r);
}  // namespace feature
