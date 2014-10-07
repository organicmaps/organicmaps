#pragma once

#include "../base/base.hpp"

#include "../std/string.hpp"


class LCG32
{
public:
  LCG32() : m_seed(31) {}
  LCG32(size_t seed) : m_seed(static_cast<uint32_t>(seed)) {}

  size_t Generate()
  {
    m_seed = m_seed * 69069 + 1;
    return m_seed;
  }

private:
  uint32_t m_seed;
};

typedef LCG32 PseudoRNG32;


namespace rnd
{
string GenerateString();
}
