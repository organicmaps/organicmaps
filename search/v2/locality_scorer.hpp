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

  // *NOTE* after call, elements of |localities| may be in any order.
  void LeaveTopLocalities(size_t limit, vector<Geocoder::Locality> & localities) const;

private:
  size_t GetTokensScore(Geocoder::Locality const & locality) const;

  RankTable const & m_rankTable;
  SearchQueryParams const & m_params;
};
}  // namespace v2
}  // namespace search
