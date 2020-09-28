#pragma once

#include "search/intermediate_result.hpp"
#include "search/nested_rects_cache.hpp"
#include "search/ranker.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/macros.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <utility>
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
    // Minimal distance between search results in mercators, needed for
    // filtering of viewport search results.
    double m_minDistanceOnMapBetweenResultsX = 0.0;
    double m_minDistanceOnMapBetweenResultsY = 0.0;

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

    // Batch size for Everywhere search mode. For viewport search we limit search results number
    // with SweepNearbyResults.
    size_t m_everywhereBatchSize = 100;

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

  // Computes missing fields for all pre-results.
  void FillMissingFieldsInPreResults();

  void Filter(bool viewportSearch);

  // Emit a new batch of results up the pipeline (i.e. to ranker).
  // Use |lastUpdate| to indicate that no more results will be added.
  void UpdateResults(bool lastUpdate);

  size_t Size() const { return m_results.size() + m_relaxedResults.size(); }
  size_t BatchSize() const
  {
    return m_params.m_viewportSearch ? std::numeric_limits<size_t>::max()
                                     : m_params.m_everywhereBatchSize;
  }
  size_t NumSentResults() const { return m_numSentResults; }
  bool HaveFullyMatchedResult() const { return m_haveFullyMatchedResult; }
  size_t Limit() const { return m_params.m_limit; }

  template <typename Fn>
  void ForEach(Fn && fn)
  {
    std::for_each(m_results.begin(), m_results.end(), std::forward<Fn>(fn));
  }

  void ClearCaches();

private:
  void FilterForViewportSearch();

  void FilterRelaxedResults(bool lastUpdate);

  DataSource const & m_dataSource;
  Ranker & m_ranker;
  std::vector<PreRankerResult> m_results;
  std::vector<PreRankerResult> m_relaxedResults;
  Params m_params;

  // Amount of results sent up the pipeline.
  size_t m_numSentResults = 0;

  // True iff there is at least one result with all tokens used (not relaxed).
  bool m_haveFullyMatchedResult = false;

  // Cache of nested rects used to estimate distance from a feature to the pivot.
  NestedRectsCache m_pivotFeatures;

  // A set of ids for features that are emitted during the current search session.
  std::set<FeatureID> m_currEmit;

  // A set of ids for features that were emitted during the previous
  // search session.  They're used for filtering of current search in
  // viewport results, because we need to give more priority to
  // results that were on map previously.
  std::set<FeatureID> m_prevEmit;

  std::minstd_rand m_rng;

  DISALLOW_COPY_AND_MOVE(PreRanker);
};
}  // namespace search
