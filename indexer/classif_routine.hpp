#pragma once

#include "../std/string.hpp"

namespace classificator
{
  void Read(string const & dir);
  void GenerateAndWrite(string const & dir);
  void PrepareForFeatureGeneration();
}
