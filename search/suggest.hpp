#pragma once

#include "search/common.hpp"
#include "search/intermediate_result.hpp"

#include "base/string_utils.hpp"

#include <cstdint>
#include <string>

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

std::string GetSuggestion(RankerResult const & res, std::string const & query,
                          QueryTokens const & paramTokens, strings::UniString const & prefix);
}  // namespace search
