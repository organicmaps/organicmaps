#include "testing/testing.hpp"

#include "search/dummy_rank_table.hpp"
#include "search/search_delimiters.hpp"
#include "search/search_string_utils.hpp"
#include "search/v2/locality_scorer.hpp"

#include "base/string_utils.hpp"

#include "std/set.hpp"
#include "std/vector.hpp"

using namespace strings;
using namespace search;
using namespace search::v2;

namespace
{
void InitParams(string const & query, SearchQueryParams & params)
{
  params.m_tokens.clear();
  params.m_prefixTokens.clear();

  vector<UniString> tokens;

  Delimiters delims;
  SplitUniString(NormalizeAndSimplifyString(query), MakeBackInsertFunctor(tokens), delims);
  for (auto const & token : tokens)
    params.m_tokens.push_back({token});
}

void AddLocality(string const & name, uint32_t featureId, SearchQueryParams & params,
                 vector<Geocoder::Locality> & localities)
{
  set<UniString> tokens;

  Delimiters delims;
  SplitUniString(NormalizeAndSimplifyString(name), [&tokens](UniString const & token)
                 {
                   tokens.insert(token);
                 },
                 delims);

  for (size_t startToken = 0; startToken < params.m_tokens.size(); ++startToken)
  {
    for (size_t endToken = startToken + 1; endToken <= params.m_tokens.size(); ++endToken)
    {
      bool matches = true;
      for (size_t i = startToken; i != endToken && matches; ++i)
      {
        if (tokens.count(params.m_tokens[i].front()) == 0)
          matches = false;
      }
      if (matches)
        localities.emplace_back(featureId, startToken, endToken);
    }
  }
}
}  // namespace

UNIT_TEST(LocalityScorer_Smoke)
{
  enum
  {
    ID_NEW_ORLEANS,
    ID_YORK,
    ID_NEW_YORK,
  };

  SearchQueryParams params;
  InitParams("New York Time Square", params);

  vector<Geocoder::Locality> localities;

  AddLocality("New Orleans", ID_NEW_ORLEANS, params, localities);
  AddLocality("York", ID_YORK, params, localities);
  AddLocality("New York", ID_NEW_YORK, params, localities);

  LocalityScorer scorer(DummyRankTable(), params);
  scorer.LeaveTopLocalities(100 /* limit */, localities);

  TEST_EQUAL(3, localities.size(), ());

  TEST_EQUAL(localities[0].m_featureId, ID_NEW_ORLEANS, ());
  TEST_EQUAL(localities[1].m_featureId, ID_YORK, ());
  TEST_EQUAL(localities[2].m_featureId, ID_NEW_YORK, ());

  // New York is the best matching locality
  scorer.LeaveTopLocalities(1 /* limit */, localities);
  TEST_EQUAL(1, localities.size(), ());
  TEST_EQUAL(localities[0].m_featureId, ID_NEW_YORK, ());
}

UNIT_TEST(LocalityScorer_NumbersMatching)
{
  enum
  {
    ID_MARCH,
    ID_APRIL,
    ID_MAY,
    ID_TVER
  };

  SearchQueryParams params;
  InitParams("тверь советская 1", params);

  vector<Geocoder::Locality> localities;

  AddLocality("поселок 1 марта", ID_MARCH, params, localities);
  AddLocality("поселок 1 апреля", ID_APRIL, params, localities);
  AddLocality("поселок 1 мая", ID_MAY, params, localities);
  AddLocality("тверь", ID_TVER, params, localities);

  LocalityScorer scorer(DummyRankTable(), params);
  scorer.LeaveTopLocalities(100 /* limit */, localities);

  TEST_EQUAL(4, localities.size(), ());

  TEST_EQUAL(localities[0].m_featureId, ID_MARCH, ());
  TEST_EQUAL(localities[1].m_featureId, ID_APRIL, ());
  TEST_EQUAL(localities[2].m_featureId, ID_MAY, ());
  TEST_EQUAL(localities[3].m_featureId, ID_TVER, ());

  // Tver is the best matching locality, as other localities were
  // matched by number.
  scorer.LeaveTopLocalities(1 /* limit */, localities);
  TEST_EQUAL(1, localities.size(), ());
  TEST_EQUAL(localities[0].m_featureId, ID_TVER, ());
}
