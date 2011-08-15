#pragma once

#include "../coding/reader.hpp"

#include "../std/string.hpp"


namespace classificator
{
  typedef ReaderPtr<Reader> file_t;
  void Read(file_t const & rules,
            file_t const & classificator,
            file_t const & visibility,
            file_t const & types);
  void ReadVisibility(string const & fPath);
}
