#pragma once

#include <random>
#include <vector>

namespace my
{
std::vector<size_t> RandomSample(size_t n, size_t k, std::minstd_rand & rng);
}  // my
