#pragma once

#include "../coding/reader.hpp"

#include "../std/string.hpp"


namespace classificator
{
  typedef ReaderPtr<Reader> ReaderType;

  void ReadVisibility(string const & fPath);

  void Load();
}
