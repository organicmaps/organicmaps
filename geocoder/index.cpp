#include "geocoder/index.hpp"

#include "geocoder/types.hpp"

#include "indexer/search_string_utils.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
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
  , m_hierarchy{hierarchy}
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
  if (tokens.size() == 1 || is_sorted(begin(tokens), end(tokens)))
    return strings::JoinStrings(tokens, " ");

  auto indexTokens = tokens;
  sort(begin(indexTokens), end(indexTokens));
  return strings::JoinStrings(indexTokens, " ");
}

void Index::AddEntries()
{
  size_t numIndexed = 0;
  auto const & dictionary = m_hierarchy.GetNormalizedNameDictionary();
  Tokens tokens;
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
      for (auto const & name : doc.GetNormalizedMultipleNames(doc.m_type, dictionary))
      {
        search::NormalizeAndTokenizeAsUtf8(name, tokens);
        InsertToIndex(tokens, docId);
      }
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

  auto isStreetSynonym = [] (string const & s) {
    return search::IsStreetSynonym(strings::MakeUniString(s));
  };

  auto const & dictionary = m_hierarchy.GetNormalizedNameDictionary();
  Tokens tokens;
  for (auto const & name : doc.GetNormalizedMultipleNames(Type::Street, dictionary))
  {
    search::NormalizeAndTokenizeAsUtf8(name, tokens);

    if (all_of(begin(tokens), end(tokens), isStreetSynonym))
    {
      if (tokens.size() > 1)
        InsertToIndex(tokens, docId);
      return;
    }

    InsertToIndex(tokens, docId);

    for (size_t i = 0; i < tokens.size(); ++i)
    {
      if (!isStreetSynonym(tokens[i]))
        continue;
      auto addr = tokens;
      addr.erase(addr.begin() + i);
      InsertToIndex(addr, docId);
    }
  }
}

void Index::AddHouses(unsigned int loadThreadsCount)
{
  atomic<size_t> numIndexed{0};
  mutex buildingsMutex;

  vector<thread> threads(loadThreadsCount);
  CHECK_GREATER(threads.size(), 0, ());

  auto const & dictionary = m_hierarchy.GetNormalizedNameDictionary();

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

        auto const & street = buildingDoc.m_normalizedAddress[static_cast<size_t>(Type::Street)];
        auto const & locality =
            buildingDoc.m_normalizedAddress[static_cast<size_t>(Type::Locality)];

        NameDictionary::Position relation = NameDictionary::kUnspecifiedPosition;
        if (street != NameDictionary::kUnspecifiedPosition)
          relation = street;
        else if (locality != NameDictionary::kUnspecifiedPosition)
          relation = locality;
        else
          continue;

        auto const & relationMultipleNames = dictionary.Get(relation);
        auto const & relationName = relationMultipleNames.GetMainName();
        Tokens relationNameTokens;
        search::NormalizeAndTokenizeAsUtf8(relationName, relationNameTokens);
        CHECK(!relationNameTokens.empty(), ());

        bool indexed = false;
        ForEachDocId(relationNameTokens, [&](DocId const & candidate) {
          auto const & candidateDoc = GetDoc(candidate);
          if (m_hierarchy.IsParentTo(candidateDoc, buildingDoc))
          {
            indexed = true;

            lock_guard<mutex> lock(buildingsMutex);
            m_relatedBuildings[candidate].emplace_back(docId);
          }
        });

        if (indexed)
        {
          auto const processedCount = numIndexed.fetch_add(1) + 1;
          if (processedCount % kLogBatch == 0)
            LOG(LINFO, ("Indexed", processedCount, "houses"));
        }
      }
    });
  }

  for (auto & t : threads)
    t.join();

  if (numIndexed % kLogBatch != 0)
    LOG(LINFO, ("Indexed", numIndexed, "houses"));
}

void Index::InsertToIndex(Tokens const & tokens, DocId docId)
{
  auto & ids = m_docIdsByTokens[MakeIndexKey(tokens)];
  if (0 == count(ids.begin(), ids.end(), docId))
    ids.emplace_back(docId);
}
}  // namespace geocoder
