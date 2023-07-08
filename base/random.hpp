#pragma once

#include <random>
#include <vector>

namespace base
{
// Selects a fair random subset of size min(|n|, |k|) from [0, 1, 2, ..., n - 1].
std::vector<size_t> RandomSample(size_t n, size_t k, std::minstd_rand & rng);
}  // base
