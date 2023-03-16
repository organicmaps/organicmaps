#include "base/random.hpp"

#include <algorithm>
#include <numeric>

namespace base
{
std::vector<size_t> RandomSample(size_t n, size_t k, std::minstd_rand & rng)
{
  std::vector<size_t> result(std::min(k, n));
  std::iota(result.begin(), result.end(), 0);

  for (size_t i = k; i < n; ++i)
  {
    size_t const j = rng() % (i + 1);
    if (j < k)
      result[j] = i;
  }

  return result;
}
}  // base
