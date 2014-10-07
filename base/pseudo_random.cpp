#include "pseudo_random.hpp"


namespace rnd
{

string GenerateString()
{
  static PseudoRNG32 rng;

  size_t const size = rng.Generate() % 20 + 1;
  string result;
  result.reserve(size);

  for (size_t i = 0; i < size; ++i)
    result.append(1, static_cast<char>(rng.Generate() + 1));
  return result;
}

}
