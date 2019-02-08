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
bool Hierarchy::Entry::DeserializeFromJSON(string const & jsonStr, ParsingStats & stats)
{
  try
  {
    base::Json root(jsonStr.c_str());
    return DeserializeFromJSONImpl(root.get(), jsonStr, stats);
  }
  catch (base::Json::Exception const & e)
  {
    LOG(LDEBUG, ("Can't parse entry:", e.Msg(), jsonStr));
  }
  return false;
}

// todo(@m) Factor out to geojson.hpp? Add geojson to myjansson?
bool Hierarchy::Entry::DeserializeFromJSONImpl(json_t * const root, string const & jsonStr,
                                               ParsingStats & stats)
{
  if (!json_is_object(root))
  {
    ++stats.m_badJsons;
    MYTHROW(base::Json::Exception, ("Not a json object."));
  }

  json_t * properties = nullptr;
  FromJSONObject(root, "properties", properties);
  json_t * address = nullptr;
  FromJSONObject(properties, "address", address);

  bool hasDuplicateAddress = false;

  for (size_t i = 0; i < static_cast<size_t>(Type::Count); ++i)
  {
    Type const type = static_cast<Type>(i);
    string const & levelKey = ToString(type);
    json_t * levelJson = nullptr;
    FromJSONObjectOptionalField(address, levelKey, levelJson);
    if (!levelJson)
      continue;

    if (base::JSONIsNull(levelJson))
      return false;

    string levelValue;
    FromJSON(levelJson, levelValue);
    if (levelValue.empty())
      continue;

    if (!m_address[i].empty())
    {
      LOG(LDEBUG, ("Duplicate address field", type, "when parsing", jsonStr));
      hasDuplicateAddress = true;
    }

    search::NormalizeAndTokenizeAsUtf8(levelValue, m_address[i]);

    if (!m_address[i].empty())
      m_type = static_cast<Type>(i);
  }

  m_nameTokens.clear();
  FromJSONObjectOptionalField(properties, "name", m_name);
  search::NormalizeAndTokenizeAsUtf8(m_name, m_nameTokens);

  if (m_name.empty())
    ++stats.m_emptyNames;

  if (hasDuplicateAddress)
    ++stats.m_duplicateAddresses;

  if (m_type == Type::Count)
  {
    LOG(LDEBUG, ("No address in an hierarchy entry:", jsonStr));
    ++stats.m_emptyAddresses;
  }
  else if (m_nameTokens != m_address[static_cast<size_t>(m_type)])
  {
    ++stats.m_mismatchedNames;
  }

  return true;
}

bool Hierarchy::Entry::IsParentTo(Hierarchy::Entry const & e) const
{
  for (size_t i = 0; i < static_cast<size_t>(geocoder::Type::Count); ++i)
  {
    if (!m_address[i].empty() && m_address[i] != e.m_address[i])
      return false;
  }
  return true;
}

// Hierarchy ---------------------------------------------------------------------------------------
Hierarchy::Hierarchy(vector<Entry> && entries, bool sorted)
  : m_entries{std::move(entries)}
{
  if (!sorted)
  {
    LOG(LINFO, ("Sorting entries..."));
    sort(m_entries.begin(), m_entries.end());
  }
}

vector<Hierarchy::Entry> const & Hierarchy::GetEntries() const
{
  return m_entries;
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
}  // namespace geocoder
