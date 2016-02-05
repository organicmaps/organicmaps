#include "search/v2/locality_scorer.hpp"

#include "search/dummy_rank_table.hpp"
#include "search/search_query_params.hpp"
#include "search/v2/mwm_context.hpp"

#include "indexer/feature_impl.hpp"
#include "indexer/index.hpp"
#include "indexer/rank_table.hpp"

#include "std/algorithm.hpp"
#include "std/unique_ptr.hpp"

namespace search
{
namespace v2
{
LocalityScorer::LocalityScorer(RankTable const & rankTable, SearchQueryParams const & params)
  : m_rankTable(rankTable), m_params(params)
{
}

void LocalityScorer::LeaveTopLocalities(size_t limit, vector<Geocoder::Locality> & localities) const
{
  // Unique localities by featureId but leave the longest range if equal.
  sort(localities.begin(), localities.end(), [&](Geocoder::Locality const & lhs, Geocoder::Locality const & rhs)
       {
         if (lhs.m_featureId != rhs.m_featureId)
           return lhs.m_featureId < rhs.m_featureId;
         return GetTokensScore(lhs) > GetTokensScore(rhs);
       });
  localities.erase(unique(localities.begin(), localities.end(),
                          [](Geocoder::Locality const & lhs, Geocoder::Locality const & rhs)
                          {
                            return lhs.m_featureId == rhs.m_featureId;
                          }),
                   localities.end());

  // Leave the most popular localities.
  /// @todo Calculate match costs according to the exact locality name
  /// (for 'york' query "york city" is better than "new york").
  sort(localities.begin(), localities.end(),
       [&](Geocoder::Locality const & lhs, Geocoder::Locality const & rhs)
       {
         auto const ls = GetTokensScore(lhs);
         auto const rs = GetTokensScore(rhs);
         if (ls != rs)
           return ls > rs;
         return m_rankTable.Get(lhs.m_featureId) > m_rankTable.Get(rhs.m_featureId);
       });
  if (localities.size() > limit)
    localities.resize(limit);
}

size_t LocalityScorer::GetTokensScore(Geocoder::Locality const & locality) const
{
  // *NOTE*
  // * full token match costs 2
  // * prefix match costs 1
  //
  // If locality is matched only by a single integral token or by an
  // integral token + a prefix, overall score is reduced by one.
  //
  // TODO (@y, @m, @vng): consider to loop over all non-prefix
  // tokens and decrement overall score by one for each integral
  // token.
  size_t const numTokens = locality.m_endToken - locality.m_startToken;
  bool const prefixMatch = locality.m_endToken == m_params.m_tokens.size() + 1;

  size_t score = 2 * numTokens;
  if (prefixMatch)
    --score;

  if ((numTokens == 2 && prefixMatch) || (numTokens == 1 && !prefixMatch))
  {
    auto const & token = m_params.GetTokens(locality.m_startToken).front();
    if (feature::IsNumber(token))
      --score;
  }

  return score;
}
}  // namespace v2
}  // namespace search
