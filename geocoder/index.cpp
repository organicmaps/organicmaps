#include "geocoder/index.hpp"

#include "geocoder/types.hpp"

#include "indexer/search_string_utils.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

using namespace std;

namespace
{
// Information will be logged for every |kLogBatch| entries.
size_t const kLogBatch = 100000;

string MakeIndexKey(geocoder::Tokens const & tokens) { return strings::JoinStrings(tokens, " "); }
}  // namespace

namespace geocoder
{
Index::Index(Hierarchy const & hierarchy) : m_entries(hierarchy.GetEntries())
{
  LOG(LINFO, ("Indexing entries..."));
  AddEntries();
  LOG(LINFO, ("Indexing houses..."));
  AddHouses();
}

vector<Index::EntryPtr> const * const Index::GetEntries(Tokens const & tokens) const
{
  auto const it = m_entriesByTokens.find(MakeIndexKey(tokens));
  if (it == m_entriesByTokens.end())
    return {};

  return &it->second;
}

vector<Index::EntryPtr> const * const Index::GetBuildingsOnStreet(
    base::GeoObjectId const & osmId) const
{
  auto const it = m_buildingsOnStreet.find(osmId);
  if (it == m_buildingsOnStreet.end())
    return {};

  return &it->second;
}

void Index::AddEntries()
{
  size_t numIndexed = 0;
  for (auto const & e : m_entries)
  {
    // The entry is indexed only by its address.
    // todo(@m) Index it by name too.
    if (e.m_type == Type::Count)
      continue;

    if (e.m_type == Type::Street)
    {
      AddStreet(e);
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

void Index::AddStreet(Hierarchy::Entry const & e)
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

void Index::AddHouses()
{
  size_t numIndexed = 0;
  for (auto & be : m_entries)
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
        m_buildingsOnStreet[se->m_osmId].emplace_back(&be);
    }

    ++numIndexed;
    if (numIndexed % kLogBatch == 0)
      LOG(LINFO, ("Indexed", numIndexed, "houses"));
  }

  if (numIndexed % kLogBatch != 0)
    LOG(LINFO, ("Indexed", numIndexed, "houses"));
}
}  // namespace geocoder
