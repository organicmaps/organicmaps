#include "geocoder/index.hpp"

#include "geocoder/types.hpp"

#include "indexer/search_string_utils.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <atomic>
#include <cstddef>
#include <mutex>
#include <thread>

using namespace std;

namespace
{
// Information will be logged for every |kLogBatch| docs.
size_t const kLogBatch = 100000;
}  // namespace

namespace geocoder
{
Index::Index(Hierarchy const & hierarchy, unsigned int loadThreadsCount)
  : m_docs(hierarchy.GetEntries())
{
  CHECK_GREATER_OR_EQUAL(loadThreadsCount, 1, ());

  LOG(LINFO, ("Indexing hierarchy entries..."));
  AddEntries();
  LOG(LINFO, ("Indexing houses..."));
  AddHouses(loadThreadsCount);
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

  auto isStreetSuffix = [] (std::string const & s) {
    return search::IsStreetSynonym(strings::MakeUniString(s));
  };

  if (all_of(begin(doc.m_address[t]), end(doc.m_address[t]), isStreetSuffix))
  {
    LOG(LDEBUG, ("Undefined proper name in tokens ", doc.m_address[t], "of street entry",
                 doc.m_osmId, "(", doc.m_address, ")"));
    if (doc.m_address[t].size() > 1)
      m_docIdsByTokens[MakeIndexKey(doc.m_address[t])].emplace_back(docId);
    return;
  }

  m_docIdsByTokens[MakeIndexKey(doc.m_address[t])].emplace_back(docId);

  for (size_t i = 0; i < doc.m_address[t].size(); ++i)
  {
    if (!isStreetSuffix(doc.m_address[t][i]))
      continue;
    auto addr = doc.m_address[t];
    addr.erase(addr.begin() + i);
    m_docIdsByTokens[MakeIndexKey(addr)].emplace_back(docId);
  }
}

void Index::AddHouses(unsigned int loadThreadsCount)
{
  atomic<size_t> numIndexed{0};
  mutex buildingsMutex;

  vector<thread> threads(loadThreadsCount);
  CHECK_GREATER(threads.size(), 0, ());

  for (size_t t = 0; t < threads.size(); ++t)
  {
    threads[t] = thread([&, t, this]() {
      size_t const size = m_docs.size() / threads.size();
      size_t docId = t * size;
      size_t const docIdEnd = (t + 1 == threads.size() ? m_docs.size() : docId + size);

      for (; docId < docIdEnd; ++docId)
      {
        auto const & buildingDoc = GetDoc(docId);

        if (buildingDoc.m_type != Type::Building)
          continue;

        auto const & street = buildingDoc.m_address[static_cast<size_t>(Type::Street)];
        auto const & locality = buildingDoc.m_address[static_cast<size_t>(Type::Locality)];

        Tokens const * relationName = nullptr;

        if (!street.empty())
          relationName = &street;
        else if (!locality.empty())
          relationName = &locality;

        if (!relationName)
          continue;

        ForEachDocId(*relationName, [&](DocId const & candidate) {
          auto const & candidateDoc = GetDoc(candidate);
          if (candidateDoc.IsParentTo(buildingDoc))
          {
            lock_guard<mutex> lock(buildingsMutex);
            m_relatedBuildings[candidate].emplace_back(docId);
          }
        });

        auto processedCount = numIndexed.fetch_add(1) + 1;
        if (processedCount % kLogBatch == 0)
          LOG(LINFO, ("Indexed", processedCount, "houses"));
      }
    });
  }

  for (auto & t : threads)
    t.join();

  if (numIndexed % kLogBatch != 0)
    LOG(LINFO, ("Indexed", numIndexed, "houses"));
}
}  // namespace geocoder
