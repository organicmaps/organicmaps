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
bool Hierarchy::Entry::DeserializeFromJSON(string const & jsonStr)
{
  try
  {
    my::Json root(jsonStr.c_str());
    DeserializeFromJSONImpl(root.get(), jsonStr);
    return true;
  }
  catch (my::Json::Exception const & e)
  {
    LOG(LWARNING, ("Can't parse entry:", e.Msg(), jsonStr));
  }
  return false;
}

// todo(@m) Factor out to geojson.hpp? Add geojson to myjansson?
void Hierarchy::Entry::DeserializeFromJSONImpl(json_t * const root, string const & jsonStr)
{
  if (!json_is_object(root))
    MYTHROW(my::Json::Exception, ("Not a json object."));

  json_t * const properties = my::GetJSONObligatoryField(root, "properties");

  json_t * const address = my::GetJSONObligatoryField(properties, "address");

  for (size_t i = 0; i < static_cast<size_t>(Type::Count); ++i)
  {
    Type const type = static_cast<Type>(i);
    string const & levelKey = ToString(type);
    string levelValue;
    FromJSONObjectOptionalField(address, levelKey, levelValue);
    if (levelValue.empty())
      continue;

    if (!m_address[i].empty())
      LOG(LWARNING, ("Duplicate address field", type, "when parsing", jsonStr));
    search::NormalizeAndTokenizeString(levelValue, m_address[i]);

    if (!m_address[i].empty())
      m_type = static_cast<Type>(i);
  }

  SetName(properties, jsonStr);

  if (m_type == Type::Count)
  {
    LOG(LWARNING, ("No address in an hierarchy entry:", jsonStr));
  }
  else
  {
    if (m_nameTokens != m_address[static_cast<size_t>(m_type)])
    {
      LOG(LWARNING, ("Hierarchy entry name is not the most detailed field in its address. Name:",
                     m_name, "Name tokens:", m_nameTokens, "Address:", m_address));
    }
  }
}

void Hierarchy::Entry::SetName(json_t * const properties, string const & jsonStr)
{
  m_nameTokens.clear();
  FromJSONObjectOptionalField(properties, "name", m_name);
  if (!m_name.empty())
  {
    search::NormalizeAndTokenizeString(m_name, m_nameTokens);
    return;
  }

  LOG(LWARNING, ("Hierarchy entry has no name. Trying to set name from address.", jsonStr));
  if (m_type != Type::Count)
    m_nameTokens = m_address[static_cast<size_t>(m_type)];
  if (m_name.empty())
    LOG(LWARNING, ("Hierarchy entry has no name and no address:", jsonStr));
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
    int64_t encodedId;
    if (i == string::npos || !strings::to_any(line.substr(0, i), encodedId))
    {
      LOG(LWARNING, ("Cannot read osm id. Line:", line));
      continue;
    }
    line = line.substr(i + 1);

    Entry entry;
    // todo(@m) We should really write uints as uints.
    entry.m_osmId = osm::Id(static_cast<uint64_t>(encodedId));

    if (!entry.DeserializeFromJSON(line))
      continue;

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
}  // namespace geocoder
