#pragma once

#include "search/intermediate_result.hpp"
#include "search/nested_rects_cache.hpp"
#include "search/ranker.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/macros.hpp"

#include "std/algorithm.hpp"
#include "std/cstdint.hpp"
#include "std/random.hpp"
#include "std/set.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

#include <boost/optional.hpp>

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
    double m_minDistanceOnMapBetweenResults = 0.0;

    // This is different from geocoder's pivot because pivot is
    // usually a rectangle created by radius and center and, due to
    // precision loss, its center may differ from
    // |m_accuratePivotCenter|. Therefore the pivot should be used for
    // fast filtering of features outside of the rectangle, while
    // |m_accuratePivotCenter| should be used when it's needed to
    // compute the distance from a feature to the pivot.
    m2::PointD m_accuratePivotCenter;

    boost::optional<m2::PointD> m_position;
    m2::RectD m_viewport;

    int m_scale = 0;

    size_t m_batchSize = 100;

    // The maximum total number of results to be emitted in all batches.
    size_t m_limit = 0;

    bool m_viewportSearch = false;
    bool m_categorialRequest = false;
  };

  PreRanker(DataSource const & dataSource, Ranker & ranker);

  void Init(Params const & params);

  void Finish(bool cancelled);

  template <typename... TArgs>
  void Emplace(TArgs &&... args)
  {
    if (m_numSentResults >= Limit())
      return;
    m_results.emplace_back(forward<TArgs>(args)...);
  }

  // Computes missing fields for all pre-results.
  void FillMissingFieldsInPreResults();

  void Filter(bool viewportSearch);

  // Emit a new batch of results up the pipeline (i.e. to ranker).
  // Use |lastUpdate| to indicate that no more results will be added.
  void UpdateResults(bool lastUpdate);

  inline size_t Size() const { return m_results.size(); }
  inline size_t BatchSize() const { return m_params.m_batchSize; }
  inline size_t NumSentResults() const { return m_numSentResults; }
  inline size_t Limit() const { return m_params.m_limit; }

  template <typename TFn>
  void ForEach(TFn && fn)
  {
    for_each(m_results.begin(), m_results.end(), forward<TFn>(fn));
  }

  void ClearCaches();

private:
  void FilterForViewportSearch();

  DataSource const & m_dataSource;
  Ranker & m_ranker;
  vector<PreRankerResult> m_results;
  Params m_params;

  // Amount of results sent up the pipeline.
  size_t m_numSentResults = 0;

  // Cache of nested rects used to estimate distance from a feature to the pivot.
  NestedRectsCache m_pivotFeatures;

  // A set of ids for features that are emitted during the current search session.
  set<FeatureID> m_currEmit;

  // A set of ids for features that were emitted during the previous
  // search session.  They're used for filtering of current search in
  // viewport results, because we need to give more priority to
  // results that were on map previously.
  set<FeatureID> m_prevEmit;

  minstd_rand m_rng;

  DISALLOW_COPY_AND_MOVE(PreRanker);
};
}  // namespace search
