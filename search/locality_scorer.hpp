#pragma once

#include "search/geocoder.hpp"
#include "search/ranking_utils.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace search
{
class CBV;
class QueryParams;
struct BaseContext;

class LocalityScorer
{
public:
  static size_t const kDefaultReadLimit;

  class Delegate
  {
  public:
    virtual ~Delegate() = default;

    virtual void GetNames(uint32_t featureId, std::vector<std::string> & names) const = 0;
    virtual uint8_t GetRank(uint32_t featureId) const = 0;
  };

  LocalityScorer(QueryParams const & params, Delegate const & delegate);

  // Leaves at most |limit| elements of |localities|, ordered by their
  // features.
  void GetTopLocalities(MwmSet::MwmId const & countryId, BaseContext const & ctx,
                        CBV const & filter, size_t limit,
                        std::vector<Geocoder::Locality> & localities);

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

  friend std::string DebugPrint(ExLocality const & locality);

  // Leaves at most |limit| elements of |localities|, ordered by some
  // combination of ranks and number of matched tokens.
  void LeaveTopLocalities(size_t limit, std::vector<Geocoder::Locality> & localities) const;

  void RemoveDuplicates(std::vector<ExLocality> & ls) const;
  void LeaveTopByRankAndProb(size_t limit, std::vector<ExLocality> & ls) const;
  void SortByNameAndProb(std::vector<ExLocality> & ls) const;

  QueryParams const & m_params;
  Delegate const & m_delegate;
};

}  // namespace search
