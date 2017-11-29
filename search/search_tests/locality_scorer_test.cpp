#include "testing/testing.hpp"

#include "search/cbv.hpp"
#include "search/geocoder_context.hpp"
#include "search/locality_scorer.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "coding/compressed_bit_vector.hpp"

#include "base/assert.hpp"
#include "base/mem_trie.hpp"
#include "base/stl_add.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include "std/algorithm.hpp"
#include "std/set.hpp"
#include "std/unordered_map.hpp"
#include "std/vector.hpp"

using namespace search;
using namespace strings;

namespace
{
class LocalityScorerTest : public LocalityScorer::Delegate
{
public:
  LocalityScorerTest() : m_scorer(m_params, static_cast<LocalityScorer::Delegate &>(*this)) {}

  void InitParams(string const & query, bool lastTokenIsPrefix)
  {
    m_params.Clear();

    vector<UniString> tokens;
    Delimiters delims;
    SplitUniString(NormalizeAndSimplifyString(query), MakeBackInsertFunctor(tokens), delims);

    if (lastTokenIsPrefix)
    {
      CHECK(!tokens.empty(), ());
      m_params.InitWithPrefix(tokens.begin(), tokens.end() - 1, tokens.back());
    }
    else
    {
      m_params.InitNoPrefix(tokens.begin(), tokens.end());
    }
  }

  void AddLocality(string const & name, uint32_t featureId)
  {
    set<UniString> tokens;
    Delimiters delims;
    SplitUniString(NormalizeAndSimplifyString(name), MakeInsertFunctor(tokens), delims);

    for (auto const & token : tokens)
      m_searchIndex.Add(token, featureId);

    m_names[featureId].push_back(name);
  }

  void GetTopLocalities(size_t limit)
  {
    BaseContext ctx;
    ctx.m_tokens.assign(m_params.GetNumTokens(), BaseContext::TOKEN_TYPE_COUNT);
    ctx.m_numTokens = m_params.GetNumTokens();

    for (size_t i = 0; i < m_params.GetNumTokens(); ++i)
    {
      auto const & token = m_params.GetToken(i);
      bool const isPrefixToken = m_params.IsPrefixToken(i);

      vector<uint64_t> ids;
      token.ForEach([&](UniString const & name) {
        if (isPrefixToken)
        {
          m_searchIndex.ForEachInSubtree(name,
                                         [&](UniString const & /* prefix */, uint32_t featureId) {
                                           ids.push_back(featureId);
                                         });
        }
        else
        {
          m_searchIndex.ForEachInNode(name, [&](uint32_t featureId) { ids.push_back(featureId); });
        }
      });

      my::SortUnique(ids);
      ctx.m_features.emplace_back(coding::CompressedBitVectorBuilder::FromBitPositions(ids));
    }

    CBV filter;
    filter.SetFull();

    m_scorer.GetTopLocalities(MwmSet::MwmId(), ctx, filter, limit, m_localities);
    sort(m_localities.begin(), m_localities.end(), my::LessBy(&Locality::m_featureId));
  }

  // LocalityScorer::Delegate overrides:
  void GetNames(uint32_t featureId, vector<string> & names) const override
  {
    auto it = m_names.find(featureId);
    if (it != m_names.end())
      names.insert(names.end(), it->second.begin(), it->second.end());
  }

  uint8_t GetRank(uint32_t /* featureId */) const override { return 0; }

protected:
  QueryParams m_params;
  vector<Locality> m_localities;
  unordered_map<uint32_t, vector<string>> m_names;
  LocalityScorer m_scorer;

  base::MemTrie<UniString, base::VectorValues<uint32_t>> m_searchIndex;
};
}  // namespace

UNIT_CLASS_TEST(LocalityScorerTest, Smoke)
{
  enum
  {
    ID_NEW_ORLEANS,
    ID_YORK,
    ID_NEW_YORK,
  };

  InitParams("New York Time Square", false /* lastTokenIsPrefix */);

  AddLocality("New Orleans", ID_NEW_ORLEANS);
  AddLocality("York", ID_YORK);
  AddLocality("New York", ID_NEW_YORK);

  GetTopLocalities(100 /* limit */);
  TEST_EQUAL(3, m_localities.size(), ());
  TEST_EQUAL(m_localities[0].m_featureId, ID_NEW_ORLEANS, ());
  TEST_EQUAL(m_localities[1].m_featureId, ID_YORK, ());
  TEST_EQUAL(m_localities[2].m_featureId, ID_NEW_YORK, ());

  // New York is the best matching locality
  GetTopLocalities(1 /* limit */);
  TEST_EQUAL(1, m_localities.size(), ());
  TEST_EQUAL(m_localities[0].m_featureId, ID_NEW_YORK, ());
}

UNIT_CLASS_TEST(LocalityScorerTest, NumbersMatch)
{
  enum
  {
    ID_MARCH,
    ID_APRIL,
    ID_MAY,
    ID_TVER
  };

  InitParams("тверь советская 1", false /* lastTokenIsPrefix */);

  AddLocality("поселок 1 марта", ID_MARCH);
  AddLocality("поселок 1 апреля", ID_APRIL);
  AddLocality("поселок 1 мая", ID_MAY);
  AddLocality("тверь", ID_TVER);

  // Tver is the only matched locality as other localities were
  // matched only by number.
  GetTopLocalities(100 /* limit */);
  TEST_EQUAL(1, m_localities.size(), ());
  TEST_EQUAL(m_localities[0].m_featureId, ID_TVER, ());

  GetTopLocalities(1 /* limit */);
  TEST_EQUAL(1, m_localities.size(), ());
  TEST_EQUAL(m_localities[0].m_featureId, ID_TVER, ());
}

UNIT_CLASS_TEST(LocalityScorerTest, NumbersComplexMatch)
{
  enum
  {
    ID_MAY,
    ID_SAINT_PETERSBURG
  };

  InitParams("saint petersburg may 1", false /* lastTokenIsPrefix */);

  AddLocality("may 1", ID_MAY);
  AddLocality("saint petersburg", ID_SAINT_PETERSBURG);

  // "May 1" contains a numeric token, but as it was matched by at
  // least two tokens, there is no penalty for numeric token. And, as
  // it has smaller featureId, it should be left.
  GetTopLocalities(1 /* limit */);
  TEST_EQUAL(1, m_localities.size(), ());
  TEST_EQUAL(m_localities[0].m_featureId, ID_MAY, ());
}

UNIT_CLASS_TEST(LocalityScorerTest, PrefixMatch)
{
  enum
  {
    ID_SAN_ANTONIO,
    ID_NEW_YORK,
    ID_YORK,
    ID_MOSCOW
  };

  InitParams("New York San Anto", true /* lastTokenIsPrefix */);

  AddLocality("San Antonio", ID_SAN_ANTONIO);
  AddLocality("New York", ID_NEW_YORK);
  AddLocality("York", ID_YORK);
  AddLocality("Moscow", ID_MOSCOW);

  // All localities except Moscow match to the search query.
  GetTopLocalities(100 /* limit */);
  TEST_EQUAL(3, m_localities.size(), ());
  TEST_EQUAL(m_localities[0].m_featureId, ID_SAN_ANTONIO, ());
  TEST_EQUAL(m_localities[1].m_featureId, ID_NEW_YORK, ());
  TEST_EQUAL(m_localities[2].m_featureId, ID_YORK, ());

  // New York and San Antonio are better than York, because they match
  // by two tokens (second token is prefix for San Antonio), whereas
  // York matches by only one token.
  GetTopLocalities(2 /* limit */);
  TEST_EQUAL(2, m_localities.size(), ());
  TEST_EQUAL(m_localities[0].m_featureId, ID_SAN_ANTONIO, ());
  TEST_EQUAL(m_localities[1].m_featureId, ID_NEW_YORK, ());

  // New York is a better than San Antonio because it matches by two
  // full tokens whereas San Antonio matches by one full token and by
  // one prefix token.
  GetTopLocalities(1 /* limit */);
  TEST_EQUAL(1, m_localities.size(), ());
  TEST_EQUAL(m_localities[0].m_featureId, ID_NEW_YORK, ());
}
