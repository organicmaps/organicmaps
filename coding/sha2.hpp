#pragma once

#include "std/string.hpp"

namespace sha2
{
  string digest224(char const * data, size_t dataSize, bool returnAsHexString);
  inline string digest224(string const & data, bool returnAsHexString = true)
  {
    return digest224(data.c_str(), data.size(), returnAsHexString);
  }

  string digest256(char const * data, size_t dataSize, bool returnAsHexString);
  inline string digest256(string const & data, bool returnAsHexString = true)
  {
    return digest256(data.c_str(), data.size(), returnAsHexString);
  }

  string digest384(char const * data, size_t dataSize, bool returnAsHexString);
  inline string digest384(string const & data, bool returnAsHexString = true)
  {
    return digest384(data.c_str(), data.size(), returnAsHexString);
  }

  string digest512(char const * data, size_t dataSize, bool returnAsHexString);
  inline string digest512(string const & data, bool returnAsHexString = true)
  {
    return digest512(data.c_str(), data.size(), returnAsHexString);
  }
}
