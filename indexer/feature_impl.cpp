#include "indexer/feature_impl.hpp"

#include "base/string_utils.hpp"
#include "base/math.hpp"

namespace feature
{
using namespace std;

bool IsHouseNumber(string const & s)
{
  return !s.empty() && strings::IsASCIIDigit(s[0]);
}

uint8_t PopulationToRank(uint64_t p)
{
  return static_cast<uint8_t>(min(0xFF, base::SignedRound(log(double(p)) / log(1.1))));
}

uint64_t RankToPopulation(uint8_t r)
{
  return static_cast<uint64_t>(pow(1.1, r));
}

} // namespace feature
