#include "search/locality_scorer.hpp"

#include "testing/testing.hpp"

#include "search/cbv.hpp"
#include "search/geocoder_context.hpp"
#include "search/ranking_utils.hpp"

#include "indexer/search_string_utils.hpp"

#include "coding/compressed_bit_vector.hpp"

#include "base/mem_trie.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace locality_scorer_test
{
using namespace search;
using namespace std;
using namespace strings;

class LocalityScorerTest : public LocalityScorer::Delegate
{
public:
  using Ids = vector<uint32_t>;

  LocalityScorerTest() : m_scorer(m_params, m2::PointD(), static_cast<LocalityScorer::Delegate &>(*this)) {}

  void InitParams(string const & query, bool lastTokenIsPrefix) { InitParams(query, m2::PointD(), lastTokenIsPrefix); }

  void InitParams(string const & query, m2::PointD const & pivot, bool lastTokenIsPrefix)
  {
    m_params.Clear();

    m_scorer.SetPivotForTesting(pivot);

    vector<UniString> tokens;
    search::ForEachNormalizedToken(query, [&tokens](strings::UniString && token)
    {
      if (!IsStopWord(token))
        tokens.push_back(std::move(token));
    });

    m_params.Init(query, tokens, lastTokenIsPrefix);
  }

  void AddLocality(string const & name, uint32_t featureId, uint8_t rank = 0, m2::PointD const & center = {},
                   bool belongsToMatchedRegion = false)
  {
    set<UniString> tokens;
    SplitUniString(NormalizeAndSimplifyString(name), base::MakeInsertFunctor(tokens), Delimiters());

    for (auto const & token : tokens)
      m_searchIndex.Add(token, featureId);

    m_names[featureId].push_back(name);
    m_ranks[featureId] = rank;
    m_centers[featureId] = center;
    m_belongsToMatchedRegion[center] = belongsToMatchedRegion;
  }

  Ids GetTopLocalities(size_t limit)
  {
    BaseContext ctx;
    size_t const numTokens = m_params.GetNumTokens();
    ctx.m_tokens.assign(numTokens, BaseContext::TOKEN_TYPE_COUNT);

    for (size_t i = 0; i < numTokens; ++i)
    {
      auto const & token = m_params.GetToken(i);
      bool const isPrefixToken = m_params.IsPrefixToken(i);

      vector<uint64_t> ids;
      token.ForOriginalAndSynonyms([&](UniString const & synonym)
      {
        if (isPrefixToken)
        {
          m_searchIndex.ForEachInSubtree(
              synonym, [&](UniString const & /* prefix */, uint32_t featureId) { ids.push_back(featureId); });
        }
        else
        {
          m_searchIndex.ForEachInNode(synonym, [&](uint32_t featureId) { ids.push_back(featureId); });
        }
      });

      base::SortUnique(ids);
      ctx.m_features.emplace_back(CBV(coding::CompressedBitVectorBuilder::FromBitPositions(ids)));
    }

    CBV filter;
    filter.SetFull();

    vector<Locality> localities;
    m_scorer.GetTopLocalities(MwmSet::MwmId(), ctx, filter, limit, localities);
    sort(localities.begin(), localities.end(), base::LessBy(&Locality::m_featureId));

    Ids ids;
    for (auto const & locality : localities)
      ids.push_back(locality.GetFeatureIndex());
    return ids;
  }

  // LocalityScorer::Delegate overrides:
  void GetNames(uint32_t featureId, vector<string> & names) const override
  {
    auto it = m_names.find(featureId);
    if (it != m_names.end())
      names.insert(names.end(), it->second.begin(), it->second.end());
  }

  uint8_t GetRank(uint32_t featureId) const override
  {
    auto it = m_ranks.find(featureId);
    return it == m_ranks.end() ? 0 : it->second;
  }

  optional<m2::PointD> GetCenter(uint32_t featureId) override
  {
    auto it = m_centers.find(featureId);
    return it == m_centers.end() ? optional<m2::PointD>() : it->second;
  }

  bool BelongsToMatchedRegion(m2::PointD const & p) const override
  {
    auto it = m_belongsToMatchedRegion.find(p);
    return it == m_belongsToMatchedRegion.end() ? false : it->second;
  }

protected:
  QueryParams m_params;
  unordered_map<uint32_t, vector<string>> m_names;
  unordered_map<uint32_t, uint8_t> m_ranks;
  unordered_map<uint32_t, m2::PointD> m_centers;
  map<m2::PointD, bool> m_belongsToMatchedRegion;
  LocalityScorer m_scorer;

  base::MemTrie<UniString, base::VectorValues<uint32_t>> m_searchIndex;
};

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

  TEST_EQUAL(GetTopLocalities(100 /* limit */), Ids({ID_NEW_ORLEANS, ID_YORK, ID_NEW_YORK}), ());
  TEST_EQUAL(GetTopLocalities(2 /* limit */), Ids({ID_YORK, ID_NEW_YORK}), ());
  TEST_EQUAL(GetTopLocalities(1 /* limit */), Ids({ID_NEW_YORK}), ());
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
  TEST_EQUAL(GetTopLocalities(100 /* limit */), Ids({ID_TVER}), ());
  TEST_EQUAL(GetTopLocalities(1 /* limit */), Ids({ID_TVER}), ());
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

  TEST_EQUAL(GetTopLocalities(2 /* limit */), Ids({ID_MAY, ID_SAINT_PETERSBURG}), ());
  TEST_EQUAL(GetTopLocalities(1 /* limit */), Ids({ID_MAY}), ());
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
  TEST_EQUAL(GetTopLocalities(100 /* limit */), Ids({ID_SAN_ANTONIO, ID_NEW_YORK, ID_YORK}), ());
  TEST_EQUAL(GetTopLocalities(2 /* limit */), Ids({ID_SAN_ANTONIO, ID_NEW_YORK}), ());
  TEST_EQUAL(GetTopLocalities(1 /* limit */), Ids({ID_SAN_ANTONIO}), ());
}

UNIT_CLASS_TEST(LocalityScorerTest, Ranks)
{
  enum
  {
    ID_SAN_MARINO,
    ID_SAN_ANTONIO,
    ID_SAN_FRANCISCO
  };

  AddLocality("San Marino", ID_SAN_MARINO, 10 /* rank */);
  AddLocality("Citta di San Antonio", ID_SAN_ANTONIO, 20 /* rank */);
  AddLocality("San Francisco", ID_SAN_FRANCISCO, 30 /* rank */);

  InitParams("San", false /* lastTokenIsPrefix */);
  TEST_EQUAL(GetTopLocalities(100 /* limit */), Ids({ID_SAN_MARINO, ID_SAN_ANTONIO, ID_SAN_FRANCISCO}), ());
  TEST_EQUAL(GetTopLocalities(2 /* limit */), Ids({ID_SAN_MARINO, ID_SAN_FRANCISCO}), ());
  TEST_EQUAL(GetTopLocalities(1 /* limit */), Ids({ID_SAN_FRANCISCO}), ());
}

UNIT_CLASS_TEST(LocalityScorerTest, Similarity)
{
  enum
  {
    ID_SAN_CARLOS,
    ID_SAN_CARLOS_BARILOCHE,
    ID_SAN_CARLOS_APOQUINDO
  };

  AddLocality("San Carlos", ID_SAN_CARLOS, 20 /* rank */);
  AddLocality("San Carlos de Bariloche", ID_SAN_CARLOS_BARILOCHE, 30 /* rank */);
  AddLocality("San Carlos de Apoquindo", ID_SAN_CARLOS_APOQUINDO, 10 /* rank */);

  InitParams("San Carlos", false /* lastTokenIsPrefix */);
  TEST_EQUAL(GetTopLocalities(1 /* limit */), Ids({ID_SAN_CARLOS}), ());

  InitParams("San Carlos de Bariloche", false /* lastTokenIsPrefix */);
  TEST_EQUAL(GetTopLocalities(1 /* limit */), Ids({ID_SAN_CARLOS_BARILOCHE}), ());

  InitParams("San Carlos de Apoquindo", false /* lastTokenIsPrefix */);
  TEST_EQUAL(GetTopLocalities(1 /* limit */), Ids({ID_SAN_CARLOS_APOQUINDO}), ());
}

UNIT_CLASS_TEST(LocalityScorerTest, DistanceToPivot)
{
  enum
  {
    ID_ABERDEEN_CLOSE,
    ID_ABERDEEN_RANK1,
    ID_ABERDEEN_RANK2,
    ID_ABERDEEN_RANK3
  };

  AddLocality("Aberdeen", ID_ABERDEEN_CLOSE, 10 /* rank */, m2::PointD(11.0, 11.0));
  AddLocality("Aberdeen", ID_ABERDEEN_RANK1, 100 /* rank */, m2::PointD(0.0, 0.0));
  AddLocality("Aberdeen", ID_ABERDEEN_RANK2, 50 /* rank */, m2::PointD(2.0, 2.0));
  AddLocality("Aberdeen", ID_ABERDEEN_RANK3, 5 /* rank */, m2::PointD(4.0, 4.0));

  InitParams("Aberdeen", m2::PointD(10.0, 10.0) /* pivot */, false /* lastTokenIsPrefix */);

  // Expected order is: the closest one (ID_ABERDEEN_CLOSE) first, then sorted by rank.
  TEST_EQUAL(GetTopLocalities(1 /* limit */), Ids({ID_ABERDEEN_CLOSE}), ());
  TEST_EQUAL(GetTopLocalities(2 /* limit */), Ids({ID_ABERDEEN_CLOSE, ID_ABERDEEN_RANK1}), ());
  TEST_EQUAL(GetTopLocalities(3 /* limit */), Ids({ID_ABERDEEN_CLOSE, ID_ABERDEEN_RANK1, ID_ABERDEEN_RANK2}), ());
}

UNIT_CLASS_TEST(LocalityScorerTest, MatchedRegion)
{
  enum
  {
    ID_SPRINGFIELD_MATCHED_REGION,
    ID_SPRINGFIELD_CLOSE,
    ID_SPRINGFIELD_RANK1,
    ID_SPRINGFIELD_RANK2
  };

  AddLocality("Springfield", ID_SPRINGFIELD_MATCHED_REGION, 5 /* rank */, m2::PointD(0.0, 0.0),
              true /* belongsToMatchedRegion */);
  AddLocality("Springfield", ID_SPRINGFIELD_CLOSE, 10 /* rank */, m2::PointD(11.0, 11.0),
              false /* belongsToMatchedRegion */);
  AddLocality("Springfield", ID_SPRINGFIELD_RANK1, 100 /* rank */, m2::PointD(2.0, 2.0),
              false /* belongsToMatchedRegion */);
  AddLocality("Springfield", ID_SPRINGFIELD_RANK2, 50 /* rank */, m2::PointD(4.0, 4.0),
              false /* belongsToMatchedRegion */);

  InitParams("Springfield", m2::PointD(10.0, 10.0) /* pivot */, false /* lastTokenIsPrefix */);

  // Expected order is: the city from the matched region, then the closest one, then sorted by rank.
  TEST_EQUAL(GetTopLocalities(1 /* limit */), Ids({ID_SPRINGFIELD_MATCHED_REGION}), ());
  TEST_EQUAL(GetTopLocalities(2 /* limit */), Ids({ID_SPRINGFIELD_MATCHED_REGION, ID_SPRINGFIELD_CLOSE}), ());
  TEST_EQUAL(GetTopLocalities(3 /* limit */),
             Ids({ID_SPRINGFIELD_MATCHED_REGION, ID_SPRINGFIELD_CLOSE, ID_SPRINGFIELD_RANK1}), ());
}
}  // namespace locality_scorer_test
