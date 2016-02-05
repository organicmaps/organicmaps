#pragma once

#include "search/v2/geocoder.hpp"

#include "std/vector.hpp"

namespace search
{
class RankTable;
struct SearchQueryParams;

namespace v2
{
class LocalityScorer
{
public:
  LocalityScorer(RankTable const & rankTable, SearchQueryParams const & params);

  // After the call there will be no more than |limit| unique elements
  // in |localities|, in descending order by number of matched tokens
  // and ranks.
  void LeaveTopLocalities(size_t limit, vector<Geocoder::Locality> & localities) const;

private:
  size_t GetTokensScore(Geocoder::Locality const & locality) const;

  RankTable const & m_rankTable;
  SearchQueryParams const & m_params;
};
}  // namespace v2
}  // namespace search
