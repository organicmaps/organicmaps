#include "geocoder/hierarchy.hpp"

#include "indexer/search_string_utils.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"

#include <fstream>

using namespace std;

namespace geocoder
{
// Hierarchy::Entry --------------------------------------------------------------------------------
bool Hierarchy::Entry::DeserializeFromJSON(string const & jsonStr, ParsingStats & stats)
{
  try
  {
    my::Json root(jsonStr.c_str());
    DeserializeFromJSONImpl(root.get(), jsonStr, stats);
    return true;
  }
  catch (my::Json::Exception const & e)
  {
    LOG(LDEBUG, ("Can't parse entry:", e.Msg(), jsonStr));
  }
  return false;
}

// todo(@m) Factor out to geojson.hpp? Add geojson to myjansson?
void Hierarchy::Entry::DeserializeFromJSONImpl(json_t * const root, string const & jsonStr,
                                               ParsingStats & stats)
{
  if (!json_is_object(root))
  {
    ++stats.m_badJsons;
    MYTHROW(my::Json::Exception, ("Not a json object."));
  }

  json_t * const properties = my::GetJSONObligatoryField(root, "properties");
  json_t * const address = my::GetJSONObligatoryField(properties, "address");
  bool hasDuplicateAddress = false;

  for (size_t i = 0; i < static_cast<size_t>(Type::Count); ++i)
  {
    Type const type = static_cast<Type>(i);
    string const & levelKey = ToString(type);
    string levelValue;
    FromJSONObjectOptionalField(address, levelKey, levelValue);
    if (levelValue.empty())
      continue;

    if (!m_address[i].empty())
    {
      LOG(LDEBUG, ("Duplicate address field", type, "when parsing", jsonStr));
      hasDuplicateAddress = true;
    }
    search::NormalizeAndTokenizeString(levelValue, m_address[i]);

    if (!m_address[i].empty())
      m_type = static_cast<Type>(i);
  }

  m_nameTokens.clear();
  FromJSONObjectOptionalField(properties, "name", m_name);
  search::NormalizeAndTokenizeString(m_name, m_nameTokens);

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
    LOG(LDEBUG, ("Hierarchy entry name is not the most detailed field in its address:", jsonStr));
  }
}

// Hierarchy ---------------------------------------------------------------------------------------
Hierarchy::Hierarchy(string const & pathToJsonHierarchy)
{
  ifstream ifs(pathToJsonHierarchy);
  string line;
  ParsingStats stats;

  while (getline(ifs, line))
  {
    if (line.empty())
      continue;

    auto i = line.find(' ');
    int64_t encodedId;
    if (i == string::npos || !strings::to_any(line.substr(0, i), encodedId))
    {
      LOG(LWARNING, ("Cannot read osm id. Line:", line));
      ++stats.m_badOsmIds;
      continue;
    }
    line = line.substr(i + 1);

    Entry entry;
    // todo(@m) We should really write uints as uints.
    entry.m_osmId = base::GeoObjectId(static_cast<uint64_t>(encodedId));

    if (!entry.DeserializeFromJSON(line, stats))
      continue;

    // The entry is indexed only by its address.
    // todo(@m) Index it by name too.
    if (entry.m_type != Type::Count)
    {
      ++stats.m_numLoaded;
      size_t const t = static_cast<size_t>(entry.m_type);
      m_entries[entry.m_address[t]].emplace_back(entry);
    }
  }

  LOG(LINFO, ("Finished reading the hierarchy. Stats:"));
  LOG(LINFO, ("Entries indexed:", stats.m_numLoaded));
  LOG(LINFO, ("Corrupted json lines:", stats.m_badJsons));
  LOG(LINFO, ("Unreadable base::GeoObjectIds:", stats.m_badOsmIds));
  LOG(LINFO, ("Entries with duplicate address parts:", stats.m_duplicateAddresses));
  LOG(LINFO, ("Entries without address:", stats.m_emptyAddresses));
  LOG(LINFO, ("Entries without names:", stats.m_emptyNames));
  LOG(LINFO,
      ("Entries whose names do not match their most specific addresses:", stats.m_mismatchedNames));
  LOG(LINFO, ("(End of stats.)"));
}

vector<Hierarchy::Entry> const * const Hierarchy::GetEntries(
    vector<strings::UniString> const & tokens) const
{
  auto it = m_entries.find(tokens);
  if (it == m_entries.end())
    return {};

  return &it->second;
}
}  // namespace geocoder
