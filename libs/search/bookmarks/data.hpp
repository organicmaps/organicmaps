#pragma once

#include "indexer/search_string_utils.hpp"

#include "kml/types.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <string>
#include <vector>

namespace search
{
namespace bookmarks
{
// TODO (@m, @y): add more features for a bookmark here, i.e. address, center.
class Data
{
public:
  Data() = default;

  Data(kml::BookmarkData const & bookmarkData, std::string const & locale)
    : m_names(ExtractIndexableNames(bookmarkData, locale))
    , m_description(kml::GetDefaultStr(bookmarkData.m_description))
  {}

  template <typename Fn>
  void ForEachNameToken(Fn && fn) const
  {
    auto withDefaultLang = [&](strings::UniString const & token)
    {
      // Note that the Default Language here is not the same as in the kml library.
      // Bookmark search by locale is not supported so every name is stored
      // in the default branch of the search trie.
      fn(StringUtf8Multilang::kDefaultCode, token);
    };

    for (auto const & name : m_names)
      ForEachNormalizedToken(name, withDefaultLang);
  }

  template <typename Fn>
  void ForEachDescriptionToken(Fn && fn) const
  {
    auto withDefaultLang = [&](strings::UniString const & token) { fn(StringUtf8Multilang::kDefaultCode, token); };

    ForEachNormalizedToken(m_description, withDefaultLang);
  }

  std::vector<std::string> const & GetNames() const { return m_names; }
  std::string const & GetDescription() const { return m_description; }

private:
  std::vector<std::string> ExtractIndexableNames(kml::BookmarkData const & bookmarkData, std::string const & locale)
  {
    std::vector<std::string> names;

    // Keep both displayed names searchable, along with the existing original-name fallback.
    auto displayed = kml::GetBookmarkDisplayNames(bookmarkData, locale);
    names.emplace_back(std::move(displayed.m_primary));
    names.emplace_back(std::move(displayed.m_secondary));
    names.emplace_back(kml::GetPreferredBookmarkStr(bookmarkData.m_name, locale));

    // todo(@m) Platform's API does not allow to use |locale| here.
    names.emplace_back(kml::GetLocalizedFeatureType(bookmarkData.m_featureTypes));

    // Normalization is postponed. It is unlikely but we may still need original strings later.
    // Trimming seems harmless, though.
    for (auto & s : names)
      strings::Trim(s);

    base::SortUnique(names);
    base::EraseIf(names, [](std::string const & s) { return s.empty(); });
    return names;
  }

  // Displayed and original names, refreshed with bookmark display settings.
  std::vector<std::string> m_names;
  std::string m_description;
};

std::string DebugPrint(Data const & data);
}  // namespace bookmarks
}  // namespace search
