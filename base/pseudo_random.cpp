#include "base/pseudo_random.hpp"


namespace rnd
{

PseudoRNG32 & Instance()
{
  static PseudoRNG32 rng;
  return rng;
}

string GenerateString()
{
  size_t const size = Instance().Generate() % 20 + 1;
  string result;
  result.reserve(size);

  for (size_t i = 0; i < size; ++i)
    result.append(1, static_cast<char>(Instance().Generate() + 1));
  return result;
}

uint64_t GetRand64()
{
  uint64_t result = Instance().Generate();
  result ^= uint64_t(Instance().Generate()) << 32;
  return result;
}

}
