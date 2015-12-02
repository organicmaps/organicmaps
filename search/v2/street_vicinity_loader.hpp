#pragma once

#include "indexer/feature_covering.hpp"
#include "indexer/scale_index.hpp"

#include "coding/reader.hpp"

#include "geometry/rect2d.hpp"

#include "std/unordered_map.hpp"

class MwmValue;
class FeaturesVector;

namespace search
{
class StreetVicinityLoader
{
public:
  StreetVicinityLoader(MwmValue & value, FeaturesVector const & featuresVector,
                       double offsetMeters);

  template <typename TFn>
  void ForEachInVicinity(uint32_t featureId, int scale, TFn const & fn)
  {
    m2::RectD const rect = GetLimitRect(featureId);
    if (rect.IsEmptyInterior())
      return;

    scale = min(max(scale, m_scaleRange.first), m_scaleRange.second);
    covering::CoveringGetter coveringGetter(rect, covering::ViewportWithLowLevels);
    auto const & intervals = coveringGetter.Get(scale);
    for (auto const & interval : intervals)
      m_index.ForEachInIntervalAndScale(fn, interval.first, interval.second, scale);
  }

  inline void ClearCache() { m_cache.clear(); }

private:
  m2::RectD GetLimitRect(uint32_t featureId);

  ScaleIndex<ModelReaderPtr> m_index;
  pair<int, int> m_scaleRange;

  FeaturesVector const & m_featuresVector;

  double const m_offsetMeters;

  unordered_map<uint32_t, m2::RectD> m_cache;
};
}  // namespace search
