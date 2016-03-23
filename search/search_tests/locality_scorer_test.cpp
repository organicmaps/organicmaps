#include "testing/testing.hpp"

#include "search/v2/locality_scorer.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/assert.hpp"
#include "base/stl_add.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include "std/algorithm.hpp"
#include "std/set.hpp"
#include "std/unordered_map.hpp"
#include "std/vector.hpp"

using namespace search::v2;
using namespace search;
using namespace strings;

namespace
{
void InitParams(string const & query, bool lastTokenIsPrefix, SearchQueryParams & params)
{
  params.m_tokens.clear();
  params.m_prefixTokens.clear();

  vector<UniString> tokens;

  Delimiters delims;
  SplitUniString(NormalizeAndSimplifyString(query), MakeBackInsertFunctor(tokens), delims);
  for (auto const & token : tokens)
    params.m_tokens.push_back({token});
  if (lastTokenIsPrefix)
  {
    ASSERT(!params.m_tokens.empty(), ());
    params.m_prefixTokens = params.m_tokens.back();
    params.m_tokens.pop_back();
  }
}

void AddLocality(string const & name, uint32_t featureId, SearchQueryParams & params,
                 vector<Geocoder::Locality> & localities)
{
  set<UniString> tokens;

  Delimiters delims;
  SplitUniString(NormalizeAndSimplifyString(name), MakeInsertFunctor(tokens), delims);

  size_t numTokens = params.m_tokens.size();
  if (!params.m_prefixTokens.empty())
    ++numTokens;

  for (size_t startToken = 0; startToken != numTokens; ++startToken)
  {
    for (size_t endToken = startToken + 1; endToken <= numTokens; ++endToken)
    {
      bool matches = true;
      for (size_t i = startToken; i != endToken && matches; ++i)
      {
        UniString const & queryToken = params.GetTokens(i).front();
        bool const isPrefix = (i == params.m_tokens.size());
        if (isPrefix)
        {
          matches = any_of(tokens.begin(), tokens.end(), [&queryToken](UniString const & token)
                           {
                             return StartsWith(token, queryToken);
                           });
        }
        else
        {
          matches = (tokens.count(queryToken) != 0);
        }
      }
      if (matches)
        localities.emplace_back(featureId, startToken, endToken);
    }
  }
}

class LocalityScorerTest : public LocalityScorer::Delegate
{
public:
  LocalityScorerTest() : m_scorer(m_params, static_cast<LocalityScorer::Delegate &>(*this)) {}

  void InitParams(string const & query, bool lastTokenIsPrefix)
  {
    ::InitParams(query, lastTokenIsPrefix, m_params);
  }

  void AddLocality(string const & name, uint32_t featureId)
  {
    ::AddLocality(name, featureId, m_params, m_localities);
    m_names[featureId].push_back(name);
  }

  void GetTopLocalities(size_t limit)
  {
    m_scorer.GetTopLocalities(limit, m_localities);
    sort(m_localities.begin(), m_localities.end(), my::CompareBy(&Geocoder::Locality::m_featureId));
  }

  // LocalityScorer::Delegate overrides:
  void GetNames(uint32_t featureId, vector<string> & names) const override
  {
    auto it = m_names.find(featureId);
    if (it != m_names.end())
      names.insert(names.end(), it->second.begin(), it->second.end());
  }

  uint8_t GetRank(uint32_t featureId) const override { return 0; }

protected:
  SearchQueryParams m_params;
  vector<Geocoder::Locality> m_localities;
  unordered_map<uint32_t, vector<string>> m_names;
  LocalityScorer m_scorer;
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

  GetTopLocalities(100 /* limit */);
  TEST_EQUAL(4, m_localities.size(), ());
  TEST_EQUAL(m_localities[0].m_featureId, ID_MARCH, ());
  TEST_EQUAL(m_localities[1].m_featureId, ID_APRIL, ());
  TEST_EQUAL(m_localities[2].m_featureId, ID_MAY, ());
  TEST_EQUAL(m_localities[3].m_featureId, ID_TVER, ());

  // Tver is the best matching locality, as other localities were
  // matched by number.
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

  // SearchQueryParams params;
  InitParams("New York San Anto", true /* lastTokenIsPrefix */);

  // vector<Geocoder::Locality> localities;
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
