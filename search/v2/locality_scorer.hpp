#pragma once

#include "search/v2/geocoder.hpp"
#include "search/v2/ranking_utils.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"

namespace search
{
struct SearchQueryParams;

namespace v2
{
class LocalityScorer
{
public:
  static size_t const kDefaultReadLimit;

  class Delegate
  {
  public:
    virtual ~Delegate() = default;

    virtual void GetNames(uint32_t featureId, vector<string> & names) const = 0;
    virtual uint8_t GetRank(uint32_t featureId) const = 0;
  };

  LocalityScorer(SearchQueryParams const & params, Delegate const & delegate);

  // Leaves at most |limit| elements of |localities|, ordered by some
  // combination of ranks and number of matched tokens.
  void GetTopLocalities(size_t limit, vector<Geocoder::Locality> & localities) const;

private:
  struct ExLocality
  {
    ExLocality();
    explicit ExLocality(Geocoder::Locality const & locality);

    inline uint32_t GetId() const { return m_locality.m_featureId; }

    Geocoder::Locality m_locality;
    size_t m_numTokens;
    uint8_t m_rank;
    NameScore m_nameScore;
  };

  void RemoveDuplicates(vector<ExLocality> & ls) const;
  void LeaveTopByRank(size_t limit, vector<ExLocality> & ls) const;
  void SortByName(vector<ExLocality> & ls) const;

  SearchQueryParams const & m_params;
  Delegate const & m_delegate;
};
}  // namespace v2
}  // namespace search
