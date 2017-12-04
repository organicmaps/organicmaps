#pragma once

#include "indexer/search_string_utils.hpp"

#include "coding/multilang_utf8_string.hpp"

#include "base/string_utils.hpp"

#include <string>

namespace search
{
namespace bookmarks
{
// TODO (@m, @y): add more features for a bookmark here, i.e. address, center.
struct Data
{
  Data() = default;

  Data(std::string const & name, std::string const & description, std::string const & type)
    : m_name(name), m_description(description), m_type(type)
  {
  }

  template <typename Fn>
  void ForEachToken(Fn && fn) const
  {
    auto withDefaultLang = [&](strings::UniString const & token) {
      fn(StringUtf8Multilang::kDefaultCode, token);
    };

    ForEachNormalizedToken(m_name, withDefaultLang);
    ForEachNormalizedToken(m_description, withDefaultLang);
    ForEachNormalizedToken(m_type, withDefaultLang);
  }

  std::string m_name;
  std::string m_description;
  std::string m_type;
};
}  // namespace bookmarks
}  // namespace search
