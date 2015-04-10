#pragma once

#include "std/string.hpp"
#include "std/vector.hpp"

namespace pl
{
  void EnumerateFilesByRegExp(string const & directory, string const & regexp,
                              vector<string> & res);
}
