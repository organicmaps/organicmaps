#pragma once

#include "../coding/reader.hpp"

#include "../std/string.hpp"


namespace classificator
{
  typedef ReaderPtr<Reader> ReaderType;

  void Read(ReaderType const & rules,
            ReaderType const & classificator,
            ReaderType const & visibility,
            ReaderType const & types);
  void ReadVisibility(string const & fPath);
}
