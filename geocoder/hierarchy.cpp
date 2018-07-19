#include "geocoder/hierarchy.hpp"

#include "indexer/search_string_utils.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"

#include <fstream>

using namespace std;

namespace
{
using EntryType = geocoder::Hierarchy::EntryType;

map<string, EntryType> const kKnownLevels = {
    {"country", EntryType::Country},
    {"region", EntryType::Region},
    {"subregion", EntryType::Subregion},
    {"locality", EntryType::Locality},
    {"sublocality", EntryType::Sublocality},
    {"suburb", EntryType::Suburb},
    {"building", EntryType::Building},
};
}  // namespace

namespace geocoder
{
// Hierarchy::Entry --------------------------------------------------------------------------------
bool Hierarchy::Entry::DeserializeFromJSON(string const & jsonStr)
{
  try
  {
    my::Json root(jsonStr.c_str());
    DeserializeFromJSONImpl(root.get());
    return true;
  }
  catch (my::Json::Exception const & e)
  {
    LOG(LWARNING, ("Can't parse entry:", e.Msg(), jsonStr));
  }
  return false;
}

// todo(@m) Factor out to geojson.hpp? Add geojson to myjansson?
void Hierarchy::Entry::DeserializeFromJSONImpl(json_t * root)
{
  if (!json_is_object(root))
    MYTHROW(my::Json::Exception, ("Not a json object."));

  json_t * const properties = my::GetJSONObligatoryField(root, "properties");

  FromJSONObject(properties, "name", m_name);
  m_nameTokens.clear();
  search::NormalizeAndTokenizeString(m_name, m_nameTokens);

  json_t * const address = my::GetJSONObligatoryField(properties, "address");

  for (auto const & e : kKnownLevels)
  {
    string const & levelKey = e.first;
    string levelValue;
    FromJSONObjectOptionalField(address, levelKey, levelValue);
    if (levelValue.empty())
      continue;

    EntryType const type = e.second;
    CHECK(m_address[static_cast<size_t>(type)].empty(), ());
    search::NormalizeAndTokenizeString(levelValue, m_address[static_cast<size_t>(type)]);
  }

  for (size_t i = 0; i < static_cast<size_t>(Hierarchy::EntryType::Count); ++i)
  {
    if (!m_address[i].empty())
      m_type = static_cast<Hierarchy::EntryType>(i);
  }

  // The name of the entry must coincide with the most detailed
  // field in its address.
  if (m_type == Hierarchy::EntryType::Count)
  {
    LOG(LWARNING,
        ("No address in an hierarchy entry. Name:", m_name, "Name tokens:", m_nameTokens));
    return;
  }
  if (m_nameTokens != m_address[static_cast<size_t>(m_type)])
  {
    LOG(LWARNING, ("Hierarchy entry name is not the most detailed field in its address. Name:",
                   m_name, "Name tokens:", m_nameTokens, "Address:", m_address));
  }
}

// Hierarchy ---------------------------------------------------------------------------------------
Hierarchy::Hierarchy(string const & pathToJsonHierarchy)
{
  ifstream ifs(pathToJsonHierarchy);
  string line;

  while (getline(ifs, line))
  {
    if (line.empty())
      continue;

    auto i = line.find(' ');
    CHECK(i != string::npos, ());
    int64_t encodedId;
    CHECK(strings::to_any(line.substr(0, i), encodedId), ());
    line = line.substr(i + 1);

    Entry entry;
    // todo(@m) We should really write uints as uints.
    entry.m_osmId = osm::Id(static_cast<uint64_t>(encodedId));

    CHECK(entry.DeserializeFromJSON(line), (line));
    m_entries[entry.m_nameTokens].emplace_back(entry);
  }

  LOG(LINFO, ("Finished reading the hierarchy"));
}

vector<Hierarchy::Entry> const * const Hierarchy::GetEntries(
    vector<strings::UniString> const & tokens) const
{
  auto it = m_entries.find(tokens);
  if (it == m_entries.end())
    return {};

  return &it->second;
}

// Functions ---------------------------------------------------------------------------------------
string DebugPrint(Hierarchy::EntryType const & type)
{
  switch (type)
  {
  case Hierarchy::EntryType::Country: return "country"; break;
  case Hierarchy::EntryType::Region: return "region"; break;
  case Hierarchy::EntryType::Subregion: return "subregion"; break;
  case Hierarchy::EntryType::Locality: return "locality"; break;
  case Hierarchy::EntryType::Sublocality: return "sublocality"; break;
  case Hierarchy::EntryType::Suburb: return "suburb"; break;
  case Hierarchy::EntryType::Building: return "building"; break;
  case Hierarchy::EntryType::Count: return "count"; break;
  }
  CHECK_SWITCH();
}
}  // namespace geocoder
