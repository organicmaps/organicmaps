#include "indexer/feature_impl.hpp"

#include <algorithm>
#include <cmath>

namespace feature
{

uint8_t PopulationToRank(uint64_t p)
{
  return std::min(0xFFl, std::lround(std::log(double(p)) / std::log(1.1)));
}

uint64_t RankToPopulation(uint8_t r)
{
  return static_cast<uint64_t>(std::pow(1.1, r));
}

}  // namespace feature
