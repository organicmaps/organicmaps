#pragma once

#include "search/intermediate_result.hpp"
#include "search/nested_rects_cache.hpp"
#include "search/ranker.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/macros.hpp"

#include <limits>
#include <optional>
#include <set>
#include <unordered_set>
#include <vector>

class DataSource;

namespace search
{
// Fast and simple pre-ranker for search results.
class PreRanker
{
public:
  struct Params
  {
    // Minimal distance between search results (by x,y axes in mercator), needed for filtering of viewport search
    // results.
    m2::PointD m_minDistanceOnMapBetweenResults{0, 0};

    // This is different from geocoder's pivot because pivot is
    // usually a rectangle created by radius and center and, due to
    // precision loss, its center may differ from
    // |m_accuratePivotCenter|. Therefore the pivot should be used for
    // fast filtering of features outside of the rectangle, while
    // |m_accuratePivotCenter| should be used when it's needed to
    // compute the distance from a feature to the pivot.
    m2::PointD m_accuratePivotCenter;

    std::optional<m2::PointD> m_position;
    m2::RectD m_viewport;

    int m_scale = 0;

    // Batch size for Everywhere search mode.
    // For viewport search we limit search results number with SweepNearbyResults.
    // Increased to 1K, no problem to read 1-2K Features per search now, but the quality is much better.
    /// @see BA_SanMartin test.
    size_t m_everywhereBatchSize = 1000;

    // The maximum total number of results to be emitted in all batches.
    size_t m_limit = 0;

    bool m_viewportSearch = false;
    bool m_categorialRequest = false;

    size_t m_numQueryTokens = 0;
  };

  PreRanker(DataSource const & dataSource, Ranker & ranker);

  void Init(Params const & params);

  void Finish(bool cancelled);

  bool IsFull() const { return m_numSentResults >= Limit() || m_ranker.IsFull(); }

  template <typename... Args>
  void Emplace(Args &&... args)
  {
    if (IsFull())
      return;

    m_results.emplace_back(std::forward<Args>(args)...);
    if (m_results.back().GetInfo().m_allTokensUsed)
      m_haveFullyMatchedResult = true;
  }

  // Emit a new batch of results up the pipeline (i.e. to ranker).
  // Use |lastUpdate| to indicate that no more results will be added.
  void UpdateResults(bool lastUpdate);

  size_t Size() const { return m_results.size() + m_relaxedResults.size(); }
  size_t BatchSize() const
  {
    return m_params.m_viewportSearch ? std::numeric_limits<size_t>::max() : m_params.m_everywhereBatchSize;
  }
  size_t NumSentResults() const { return m_numSentResults; }
  bool ContinueSearch() const { return !m_haveFullyMatchedResult || Size() < BatchSize(); }
  size_t Limit() const { return m_params.m_limit; }

  // Iterate results per-MWM clusters.
  // Made it "static template" for easy unit tests implementing.
  template <class T, class FnT>
  static void ForEachMwmOrder(std::vector<T> & vec, FnT && fn)
  {
    size_t const count = vec.size();
    if (count == 0)
      return;

    std::set<MwmSet::MwmId> processed;

    size_t next = 0;
    bool nextAssigned;

    do
    {
      fn(vec[next]);

      MwmSet::MwmId const mwmId = vec[next].GetId().m_mwmId;

      nextAssigned = false;
      for (size_t i = next + 1; i < count; ++i)
      {
        auto const & currId = vec[i].GetId().m_mwmId;
        if (currId == mwmId)
        {
          fn(vec[i]);
        }
        else if (!nextAssigned && processed.count(currId) == 0)
        {
          next = i;
          nextAssigned = true;
        }
      }

      processed.insert(mwmId);
    }
    while (nextAssigned);
  }

  void ClearCaches();

private:
  // Computes missing fields for all pre-results.
  void FillMissingFieldsInPreResults();
  void DbgFindAndLog(std::set<uint32_t> const & ids) const;

  void FilterForViewportSearch();
  void Filter();
  void FilterRelaxedResults(bool lastUpdate);

  DataSource const & m_dataSource;
  Ranker & m_ranker;

  using PreResultsContainerT = std::vector<PreRankerResult>;
  PreResultsContainerT m_results, m_relaxedResults;

  Params m_params;

  // Amount of results sent up the pipeline.
  size_t m_numSentResults = 0;

  // True iff there is at least one result with all tokens used (not relaxed).
  bool m_haveFullyMatchedResult = false;

  // Cache of nested rects used to estimate distance from a feature to the pivot.
  NestedRectsCache m_pivotFeatures;

  /// @name Only for the viewport search. Store a set of ids that were emitted during the previous
  /// search session. They're used for filtering of current search, because we need to give more priority
  /// to results that were on map previously, to avoid result's annoying blinking/flickering on map.
  /// @{
  std::unordered_set<FeatureID> m_currEmit;
  std::unordered_set<FeatureID> m_prevEmit;
  /// @}

  unsigned m_rndSeed;

  DISALLOW_COPY_AND_MOVE(PreRanker);
};
}  // namespace search
