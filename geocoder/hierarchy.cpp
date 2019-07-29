#include "geocoder/hierarchy.hpp"

#include "indexer/search_string_utils.hpp"

#include "base/exception.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <utility>

using namespace std;

namespace geocoder
{
// Hierarchy::Entry --------------------------------------------------------------------------------
bool Hierarchy::Entry::DeserializeFromJSON(string const & jsonStr,
                                           NameDictionaryMaker & normalizedNameDictionaryMaker,
                                           ParsingStats & stats)
{
  try
  {
    base::Json root(jsonStr.c_str());
    return DeserializeFromJSONImpl(root.get(), jsonStr, normalizedNameDictionaryMaker, stats);
  }
  catch (base::Json::Exception const & e)
  {
    LOG(LDEBUG, ("Can't parse entry:", e.Msg(), jsonStr));
  }
  return false;
}

// todo(@m) Factor out to geojson.hpp? Add geojson to myjansson?
bool Hierarchy::Entry::DeserializeFromJSONImpl(json_t * const root, string const & jsonStr,
                                               NameDictionaryMaker & normalizedNameDictionaryMaker,
                                               ParsingStats & stats)
{
  if (!json_is_object(root))
  {
    ++stats.m_badJsons;
    MYTHROW(base::Json::Exception, ("Not a json object."));
  }

  auto const defaultLocale = base::GetJSONObligatoryFieldByPath(root, "properties", "locales",
                                                                "default");
  auto const address = base::GetJSONObligatoryField(defaultLocale, "address");
  m_normalizedAddress= {};
  Tokens tokens;
  for (size_t i = 0; i < static_cast<size_t>(Type::Count); ++i)
  {
    Type const type = static_cast<Type>(i);
    string const & levelKey = ToString(type);
    auto const levelJson = base::GetJSONOptionalField(address, levelKey);
    if (!levelJson)
      continue;

    if (base::JSONIsNull(levelJson))
      return false;

    string levelValue;
    FromJSON(levelJson, levelValue);
    if (levelValue.empty())
      continue;

    search::NormalizeAndTokenizeAsUtf8(levelValue, tokens);
    if (tokens.empty())
      continue;

    auto normalizedValue = strings::JoinStrings(tokens, " ");
    m_normalizedAddress[i] = normalizedNameDictionaryMaker.Add(normalizedValue);
    m_type = static_cast<Type>(i);
  }

  auto const & subregion = m_normalizedAddress[static_cast<size_t>(Type::Subregion)];
  auto const & locality = m_normalizedAddress[static_cast<size_t>(Type::Locality)];
  if (m_type == Type::Street && !locality && !subregion)
  {
    ++stats.m_noLocalityStreets;
    return false;
  }
  if (m_type == Type::Building && !locality && !subregion)
  {
    ++stats.m_noLocalityBuildings;
    return false;
  }

  FromJSONObjectOptionalField(defaultLocale, "name", m_name);
  if (m_name.empty())
    ++stats.m_emptyNames;

  if (m_type == Type::Count)
  {
    LOG(LDEBUG, ("No address in an hierarchy entry:", jsonStr));
    ++stats.m_emptyAddresses;
  }
  return true;
}

std::string const & Hierarchy::Entry::GetNormalizedName(
    Type type, NameDictionary const & normalizedNameDictionary) const
{
  return normalizedNameDictionary.Get(m_normalizedAddress[static_cast<size_t>(type)]);
}

// Hierarchy ---------------------------------------------------------------------------------------
Hierarchy::Hierarchy(vector<Entry> && entries, NameDictionary && normalizedNameDictionary)
  : m_entries{move(entries)}
  , m_normalizedNameDictionary{move(normalizedNameDictionary)}
{
  if (!is_sorted(m_entries.begin(), m_entries.end()))
  {
    LOG(LINFO, ("Sorting entries..."));
    sort(m_entries.begin(), m_entries.end());
  }
}

vector<Hierarchy::Entry> const & Hierarchy::GetEntries() const
{
  return m_entries;
}

NameDictionary const & Hierarchy::GetNormalizedNameDictionary() const
{
  return m_normalizedNameDictionary;
}

Hierarchy::Entry const * Hierarchy::GetEntryForOsmId(base::GeoObjectId const & osmId) const
{
  auto const cmp = [](Hierarchy::Entry const & e, base::GeoObjectId const & id) {
    return e.m_osmId < id;
  };

  auto const it = lower_bound(m_entries.begin(), m_entries.end(), osmId, cmp);

  if (it == m_entries.end() || it->m_osmId != osmId)
    return nullptr;

  return &(*it);
}

bool Hierarchy::IsParentTo(Hierarchy::Entry const & entry, Hierarchy::Entry const & toEntry) const
{
  for (size_t i = 0; i < static_cast<size_t>(geocoder::Type::Count); ++i)
  {
    if (!entry.m_normalizedAddress[i])
      continue;

    if (!toEntry.m_normalizedAddress[i])
      return false;
    auto const pos1 = entry.m_normalizedAddress[i];
    auto const pos2 = toEntry.m_normalizedAddress[i];
    if (pos1 != pos2 &&
        m_normalizedNameDictionary.Get(pos1) != m_normalizedNameDictionary.Get(pos2))
    {
      return false;
    }
  }
  return true;
}
}  // namespace geocoder
