#pragma once

#include "search/common.hpp"

#include "base/string_utils.hpp"

namespace search
{
struct Suggest
{
  Suggest(strings::UniString const & name, uint8_t len, int8_t locale)
    : m_name(name)
    , m_prefixLength(len)
    , m_locale(locale)
  {}

  strings::UniString m_name;
  uint8_t m_prefixLength;
  int8_t m_locale;
};

std::string GetSuggestion(std::string const & name, QueryString const & query);
}  // namespace search
