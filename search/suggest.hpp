#pragma once

#include "base/string_utils.hpp"

#include "std/cstdint.hpp"

namespace search
{
struct Suggest
{
  Suggest(strings::UniString const & name, uint8_t len, int8_t locale)
    : m_name(name), m_prefixLength(len), m_locale(locale)
  {
  }

  strings::UniString m_name;
  uint8_t m_prefixLength;
  int8_t m_locale;
};
}  // namespace search
