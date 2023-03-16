#pragma once

#include "search/cbv.hpp"
#include "search/geocoder_locality.hpp"

#include <limits>
#include <optional>
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
  class Delegate
  {
  public:
    virtual ~Delegate() = default;

    virtual void GetNames(uint32_t featureId, std::vector<std::string> & names) const = 0;
    virtual uint8_t GetRank(uint32_t featureId) const = 0;
    virtual std::optional<m2::PointD> GetCenter(uint32_t featureId) = 0;
    virtual bool BelongsToMatchedRegion(m2::PointD const & p) const = 0;
  };

  LocalityScorer(QueryParams const & params, m2::PointD const & pivot, Delegate & delegate);

  void SetPivotForTesting(m2::PointD const & pivot) { m_pivot = pivot; }

  // Leaves at most |limit| elements of |localities|, ordered by their
  // features.
  void GetTopLocalities(MwmSet::MwmId const & countryId, BaseContext const & ctx,
                        CBV const & filter, size_t limit, std::vector<Locality> & localities);

private:
  struct ExLocality
  {
    ExLocality(Locality && locality, double queryNorm, uint8_t rank);

    uint32_t GetId() const { return m_locality.m_featureId; }

    Locality m_locality;
    double m_queryNorm = 0.0;
    double m_similarity = 0.0;
    uint8_t m_rank = 0;
    double m_distanceToPivot = std::numeric_limits<double>::max();
    bool m_belongsToMatchedRegion = false;
  };

  friend std::string DebugPrint(ExLocality const & locality);

  // Leaves at most |limit| elements of |localities|. Candidates are selected with
  // LeaveTopByExactMatchNormAndRank. Number of candidates typically is much bigger than |limit|
  // (100 vs 5). Then final localities are selected from candidates with
  // LeaveTopBySimilarityAndOther.
  void LeaveTopLocalities(IdfMap & idfs, size_t limit, std::vector<Locality> & localities);

  // Selects at most |limitUniqueIds| best features by exact match, query norm and
  // rank, and then leaves only localities corresponding to those
  // features in |els|.
  void LeaveTopByExactMatchNormAndRank(size_t limitUniqueIds, std::vector<ExLocality> & els) const;

  // Leaves at most |limit| unique best localities by similarity and matched tokens range size. For
  // elements with the same similarity and matched range size prefers localities from already
  // matched regions, then the closest one (by distance to pivot), rest of elements are sorted by
  // rank.
  void GroupBySimilarityAndOther(std::vector<ExLocality> & els) const;

  void GetDocVecs(uint32_t localityId, std::vector<DocVec> & dvs) const;
  double GetSimilarity(QueryVec & qv, IdfMap & docIdfs, std::vector<DocVec> & dvs) const;

  QueryParams const & m_params;
  m2::PointD m_pivot;
  Delegate & m_delegate;
};
}  // namespace search
