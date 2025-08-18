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

    // Same as GetPreferredBookmarkName from the map library. Duplicated here to avoid dependency.
    names.emplace_back(kml::GetPreferredBookmarkName(bookmarkData, locale));
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

  // Names and custom names in all the locales that we are interested in.
  // The locale set is fixed at startup and the relevant names are provided
  // by the kml library. In case the user switches the device locale while
  // running the app, the UI will adapt; however the search will not, and the
  // bookmarks will not be reindexed. We consider this situation to be improbable
  // enough to justify not storing redundant names here.
  std::vector<std::string> m_names;
  std::string m_description;
};

std::string DebugPrint(Data const & data);
}  // namespace bookmarks
}  // namespace search
