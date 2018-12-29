#include "geocoder/hierarchy.hpp"

#include "indexer/search_string_utils.hpp"

#include "base/exception.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <fstream>
#include <utility>

using namespace std;

namespace
{
// Information will be logged for every |kLogBatch| entries.
size_t const kLogBatch = 100000;

void CheckDuplicateOsmIds(vector<geocoder::Hierarchy::Entry> const & entries,
                          geocoder::Hierarchy::ParsingStats & stats)
{
  size_t i = 0;
  while (i < entries.size())
  {
    size_t j = i + 1;
    while (j < entries.size() && entries[i].m_osmId == entries[j].m_osmId)
      ++j;
    if (j != i + 1)
    {
      ++stats.m_duplicateOsmIds;
      // todo Remove the cast when the hierarchies no longer contain negative keys.
      LOG(LDEBUG,
          ("Duplicate osm id:", static_cast<int64_t>(entries[i].m_osmId.GetEncodedId()), "(",
           entries[i].m_osmId, ")", "occurs as a key in", j - i, "key-value entries."));
    }
    i = j;
  }
}
}  // namespace

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

// Hierarchy ---------------------------------------------------------------------------------------
Hierarchy::Hierarchy(string const & pathToJsonHierarchy)
{
  ifstream ifs(pathToJsonHierarchy);
  string line;
  ParsingStats stats;

  LOG(LINFO, ("Reading entries..."));
  while (getline(ifs, line))
  {
    if (line.empty())
      continue;

    auto const i = line.find(' ');
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

    if (entry.m_type == Type::Count)
      continue;

    ++stats.m_numLoaded;
    if (stats.m_numLoaded % kLogBatch == 0)
      LOG(LINFO, ("Read", stats.m_numLoaded, "entries"));

    m_entries.emplace_back(move(entry));
  }

  if (stats.m_numLoaded % kLogBatch != 0)
    LOG(LINFO, ("Read", stats.m_numLoaded, "entries"));

  LOG(LINFO, ("Sorting entries..."));
  sort(m_entries.begin(), m_entries.end());

  CheckDuplicateOsmIds(m_entries, stats);

  LOG(LINFO, ("Finished reading and indexing the hierarchy. Stats:"));
  LOG(LINFO, ("Entries loaded:", stats.m_numLoaded));
  LOG(LINFO, ("Corrupted json lines:", stats.m_badJsons));
  LOG(LINFO, ("Unreadable base::GeoObjectIds:", stats.m_badOsmIds));
  LOG(LINFO, ("Duplicate base::GeoObjectIds:", stats.m_duplicateOsmIds));
  LOG(LINFO, ("Entries with duplicate address parts:", stats.m_duplicateAddresses));
  LOG(LINFO, ("Entries without address:", stats.m_emptyAddresses));
  LOG(LINFO, ("Entries without names:", stats.m_emptyNames));
  LOG(LINFO,
      ("Entries whose names do not match their most specific addresses:", stats.m_mismatchedNames));
  LOG(LINFO, ("(End of stats.)"));
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

bool Hierarchy::IsParent(Hierarchy::Entry const & pe, Hierarchy::Entry const & e) const
{
  for (size_t i = 0; i < static_cast<size_t>(geocoder::Type::Count); ++i)
  {
    if (!pe.m_address[i].empty() && pe.m_address[i] != e.m_address[i])
      return false;
  }
  return true;
}
}  // namespace geocoder
