#include "geocoder/hierarchy.hpp"

#include "indexer/search_string_utils.hpp"

#include "base/assert.hpp"
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

string MakeIndexKey(geocoder::Tokens const & tokens)
{
  return strings::JoinStrings(tokens, " ");
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
    DeserializeFromJSONImpl(root.get(), jsonStr, stats);
    return true;
  }
  catch (base::Json::Exception const & e)
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
    MYTHROW(base::Json::Exception, ("Not a json object."));
  }

  json_t * const properties = base::GetJSONObligatoryField(root, "properties");
  json_t * const address = base::GetJSONObligatoryField(properties, "address");
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

    if (entry.m_type == Type::Count)
      continue;

    ++stats.m_numLoaded;
    if (stats.m_numLoaded % kLogBatch == 0)
      LOG(LINFO, ("Read", stats.m_numLoaded, "entries"));

    m_entriesStorage.emplace_back(move(entry));
  }

  if (stats.m_numLoaded % kLogBatch != 0)
    LOG(LINFO, ("Read", stats.m_numLoaded, "entries"));

  LOG(LINFO, ("Sorting entries..."));
  sort(m_entriesStorage.begin(), m_entriesStorage.end());

  LOG(LINFO, ("Indexing entries..."));
  IndexEntries();
  LOG(LINFO, ("Indexing houses..."));
  IndexHouses();

  LOG(LINFO, ("Finished reading and indexing the hierarchy. Stats:"));
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

vector<Hierarchy::Entry *> const * const Hierarchy::GetEntries(Tokens const & tokens) const
{
  auto it = m_entriesByTokens.find(MakeIndexKey(tokens));
  if (it == m_entriesByTokens.end())
    return {};

  return &it->second;
}

Hierarchy::Entry const * Hierarchy::GetEntryForOsmId(base::GeoObjectId const & osmId) const
{
  auto const cmp = [](Hierarchy::Entry const & e, base::GeoObjectId const & id) {
    return e.m_osmId < id;
  };

  auto it = lower_bound(m_entriesStorage.begin(), m_entriesStorage.end(), osmId, cmp);

  if (it == m_entriesStorage.end() || it->m_osmId != osmId)
    return nullptr;

  return &(*it);
}

void Hierarchy::IndexEntries()
{
  size_t numIndexed = 0;
  for (Entry & e : m_entriesStorage)
  {
    // The entry is indexed only by its address.
    // todo(@m) Index it by name too.
    if (e.m_type == Type::Count)
      continue;

    if (e.m_type == Type::Street)
    {
      IndexStreet(e);
    }
    else
    {
      size_t const t = static_cast<size_t>(e.m_type);
      m_entriesByTokens[MakeIndexKey(e.m_address[t])].emplace_back(&e);
    }

    // Index every token but do not index prefixes.
    // for (auto const & tok : entry.m_address[t])
    // 	m_entriesByTokens[{tok}].emplace_back(entry);

    ++numIndexed;
    if (numIndexed % kLogBatch == 0)
      LOG(LINFO, ("Indexed", numIndexed, "entries"));
  }

  if (numIndexed % kLogBatch != 0)
    LOG(LINFO, ("Indexed", numIndexed, "entries"));
}

void Hierarchy::IndexStreet(Entry & e)
{
  CHECK_EQUAL(e.m_type, Type::Street, ());
  size_t const t = static_cast<size_t>(e.m_type);
  m_entriesByTokens[MakeIndexKey(e.m_address[t])].emplace_back(&e);

  for (size_t i = 0; i < e.m_address[t].size(); ++i)
  {
    if (!search::IsStreetSynonym(strings::MakeUniString(e.m_address[t][i])))
      continue;
    auto addr = e.m_address[t];
    addr.erase(addr.begin() + i);
    m_entriesByTokens[MakeIndexKey(addr)].emplace_back(&e);
  }
}

void Hierarchy::IndexHouses()
{
  size_t numIndexed = 0;
  for (auto const & be : m_entriesStorage)
  {
    if (be.m_type != Type::Building)
      continue;

    size_t const t = static_cast<size_t>(Type::Street);

    auto const * streetCandidates = GetEntries(be.m_address[t]);
    if (streetCandidates == nullptr)
      continue;

    for (auto & se : *streetCandidates)
    {
      if (se->IsParentTo(be))
        se->m_buildingsOnStreet.emplace_back(&be);
    }

    ++numIndexed;
    if (numIndexed % kLogBatch == 0)
      LOG(LINFO, ("Indexed", numIndexed, "houses"));
  }

  if (numIndexed % kLogBatch != 0)
    LOG(LINFO, ("Indexed", numIndexed, "houses"));
}
}  // namespace geocoder
