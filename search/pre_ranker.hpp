#pragma once

#include "search/intermediate_result.hpp"
#include "search/nested_rects_cache.hpp"
#include "search/ranker.hpp"

#include "indexer/index.hpp"

#include "geometry/point2d.hpp"

#include "base/macros.hpp"

#include "std/algorithm.hpp"
#include "std/cstdint.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace search
{
// Fast and simple pre-ranker for search results.
class PreRanker
{
public:
  struct Params
  {
    // This is different from geocoder's pivot because pivot is
    // usually a rectangle created by radius and center and, due to
    // precision loss, its center may differ from
    // |m_accuratePivotCenter|. Therefore the pivot should be used for
    // fast filtering of features outside of the rectangle, while
    // |m_accuratePivotCenter| should be used when it's needed to
    // compute the distance from a feature to the pivot.
    m2::PointD m_accuratePivotCenter = m2::PointD(0, 0);
    int m_scale = 0;
  };

  PreRanker(Index const & index, Ranker & ranker, size_t limit);

  inline void Init(Params const & params) { m_params = params; }
  inline void SetViewportSearch(bool viewportSearch) { m_viewportSearch = viewportSearch; }
  inline void SetAccuratePivotCenter(m2::PointD const & center)
  {
    m_params.m_accuratePivotCenter = center;
  }

  template <typename... TArgs>
  void Emplace(TArgs &&... args)
  {
    m_results.emplace_back(forward<TArgs>(args)...);
  }

  // Computes missing fields for all results.
  void FillMissingFieldsInResults();

  void Filter(bool viewportSearch);

  // This function is used in geocoder to indicate that
  // no more results will be added.
  void FinalizeResults();

  inline size_t Size() const { return m_results.size(); }
  inline size_t Limit() const { return m_limit; }
  inline bool IsEmpty() const { return Size() == 0; }
  inline void Clear() { m_results.clear(); };

  template <typename TFn>
  void ForEach(TFn && fn)
  {
    for_each(m_results.begin(), m_results.end(), forward<TFn>(fn));
  }

  void ClearCaches();

private:
  Index const & m_index;
  Ranker & m_ranker;
  vector<PreResult1> m_results;
  size_t const m_limit;
  Params m_params;
  bool m_viewportSearch = false;

  // Cache of nested rects used to estimate distance from a feature to the pivot.
  NestedRectsCache m_pivotFeatures;

  DISALLOW_COPY_AND_MOVE(PreRanker);
};
}  // namespace search
