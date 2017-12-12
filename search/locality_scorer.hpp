#pragma once

#include "search/geocoder_locality.hpp"
#include "search/ranking_utils.hpp"

#include "base/string_utils.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace search
{
class CBV;
class IdfMap;
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
                        CBV const & filter, size_t limit, std::vector<Locality> & localities);

private:
  using DocVec = DocVec<strings::UniString>;
  using QueryVec = Locality::QueryVec;

  struct ExLocality
  {
    ExLocality() = default;
    ExLocality(Locality const & locality, double queryNorm, uint8_t rank);

    uint32_t GetId() const { return m_locality.m_featureId; }

    Locality m_locality;
    double m_queryNorm = 0.0;
    double m_similarity = 0.0;
    uint8_t m_rank = 0;
  };

  friend std::string DebugPrint(ExLocality const & locality);

  // Leaves at most |limit| elements of |localities|, ordered by some
  // combination of ranks and number of matched tokens.
  void LeaveTopLocalities(IdfMap const & idfs, size_t limit,
                          std::vector<Locality> & localities) const;

  // Selects at most |limitUniqueIds| best features by query norm and
  // rank, and then leaves only localities corresponding to those
  // features in |els|.
  void LeaveTopByNormAndRank(size_t limitUniqueIds, std::vector<ExLocality> & els) const;

  // Leaves at most |limit| best localities by similarity to the query
  // and rank. Result doesn't contain duplicate features.
  void LeaveTopBySimilarityAndRank(size_t limit, std::vector<ExLocality> & els) const;

  void GetDocVecs(IdfMap const & idfs, uint32_t localityId, vector<DocVec> & dvs) const;
  double GetSimilarity(QueryVec const & qv, std::vector<DocVec> const & dvs) const;

  QueryParams const & m_params;
  Delegate const & m_delegate;
};
}  // namespace search
