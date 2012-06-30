#pragma once

#include "../std/string.hpp"
#include "../std/vector.hpp"

namespace pl
{
  string GetFixedMask(string const & mask);
  void EnumerateFilesInDir(string const & directory, string const & mask, vector<string> & res);
}
