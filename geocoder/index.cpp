#include "geocoder/index.hpp"

#include "geocoder/types.hpp"

#include "indexer/search_string_utils.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <cstddef>

using namespace std;

namespace
{
// Information will be logged for every |kLogBatch| docs.
size_t const kLogBatch = 100000;
}  // namespace

namespace geocoder
{
Index::Index(Hierarchy const & hierarchy) : m_docs(hierarchy.GetEntries())
{
  LOG(LINFO, ("Indexing hierarchy entries..."));
  AddEntries();
  LOG(LINFO, ("Indexing houses..."));
  AddHouses();
}

Index::Doc const & Index::GetDoc(DocId const id) const
{
  ASSERT_LESS(static_cast<size_t>(id), m_docs.size(), ());
  return m_docs[static_cast<size_t>(id)];
}

// static
string Index::MakeIndexKey(Tokens const & tokens)
{
  return strings::JoinStrings(tokens, " ");
}

void Index::AddEntries()
{
  size_t numIndexed = 0;
  for (DocId docId = 0; docId < static_cast<DocId>(m_docs.size()); ++docId)
  {
    auto const & doc = m_docs[static_cast<size_t>(docId)];
    // The doc is indexed only by its address.
    // todo(@m) Index it by name too.
    if (doc.m_type == Type::Count)
      continue;

    if (doc.m_type == Type::Building)
      continue;

    if (doc.m_type == Type::Street)
    {
      AddStreet(docId, doc);
    }
    else
    {
      size_t const t = static_cast<size_t>(doc.m_type);
      m_docIdsByTokens[MakeIndexKey(doc.m_address[t])].emplace_back(docId);
    }

    ++numIndexed;
    if (numIndexed % kLogBatch == 0)
      LOG(LINFO, ("Indexed", numIndexed, "entries"));
  }

  if (numIndexed % kLogBatch != 0)
    LOG(LINFO, ("Indexed", numIndexed, "entries"));
}

void Index::AddStreet(DocId const & docId, Index::Doc const & doc)
{
  CHECK_EQUAL(doc.m_type, Type::Street, ());
  size_t const t = static_cast<size_t>(doc.m_type);
  m_docIdsByTokens[MakeIndexKey(doc.m_address[t])].emplace_back(docId);

  for (size_t i = 0; i < doc.m_address[t].size(); ++i)
  {
    if (!search::IsStreetSynonym(strings::MakeUniString(doc.m_address[t][i])))
      continue;
    auto addr = doc.m_address[t];
    addr.erase(addr.begin() + i);
    m_docIdsByTokens[MakeIndexKey(addr)].emplace_back(docId);
  }
}

void Index::AddHouses()
{
  size_t numIndexed = 0;
  for (DocId docId = 0; docId < static_cast<DocId>(m_docs.size()); ++docId)
  {
    auto const & buildingDoc = GetDoc(docId);

    if (buildingDoc.m_type != Type::Building)
      continue;

    auto const & street = buildingDoc.m_address[static_cast<size_t>(Type::Street)];
    auto const & locality = buildingDoc.m_address[static_cast<size_t>(Type::Locality)];

    Tokens const * buildingPlace = nullptr;

    if (!street.empty())
      buildingPlace = &street;
    else if (!locality.empty())
      buildingPlace = &locality;

    if (!buildingPlace)
      continue;

    ForEachDocId(*buildingPlace, [&](DocId const & placeCandidate) {
      auto const & streetDoc = GetDoc(placeCandidate);
      if (streetDoc.IsParentTo(buildingDoc))
      {
        m_buildingsOnStreet[placeCandidate].emplace_back(docId);

        ++numIndexed;
        if (numIndexed % kLogBatch == 0)
          LOG(LINFO, ("Indexed", numIndexed, "houses"));
      }
    });
  }

  if (numIndexed % kLogBatch != 0)
    LOG(LINFO, ("Indexed", numIndexed, "houses"));
}
}  // namespace geocoder
