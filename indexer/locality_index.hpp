#pragma once

#include "indexer/cell_id.hpp"
#include "indexer/feature_covering.hpp"
#include "indexer/interval_index.hpp"
#include "indexer/locality_object.hpp"
#include "indexer/scales.hpp"

#include "geometry/rect2d.hpp"

#include "base/osm_id.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <set>

namespace indexer
{
// Geometry index which stores osm::Id as object identifier.
// Used for geocoder server, stores only POIs and buildings which have address information.
// Based on IntervalIndex.
template <typename Reader>
class LocalityIndex
{
public:
  using ProcessObject = std::function<void(osm::Id const &)>;

  explicit LocalityIndex(Reader const & reader)
  {
    m_intervalIndex = std::make_unique<IntervalIndex<Reader, uint64_t>>(reader);
  }

  void ForEachInRect(ProcessObject const & processObject, m2::RectD const & rect) const
  {
    covering::CoveringGetter cov(rect, covering::CoveringMode::ViewportWithLowLevels);
    covering::Intervals const & intervals =
        cov.Get<LocalityCellId::DEPTH_LEVELS>(scales::GetUpperScale());

    for (auto const & i : intervals)
    {
      m_intervalIndex->ForEach(
          [&processObject](uint64_t storedId) {
            processObject(LocalityObject::FromStoredId(storedId));
          },
          i.first, i.second);
    }
  }

  // Applies |processObject| to at most |topSize| object closest to |center| with maximal distance |sizeM|.
  // Closest objects are first. Due to perfomance reasons far objects have approximate order.
  void ForClosestToPoint(ProcessObject const & processObject, m2::PointD const & center, double sizeM,
                         uint32_t topSize) const
  {
    auto const rect =
        MercatorBounds::RectByCenterXYAndSizeInMeters(center, sizeM);
    covering::CoveringGetter cov(rect, covering::CoveringMode::Spiral);
    covering::Intervals const & intervals =
        cov.Get<LocalityCellId::DEPTH_LEVELS>(scales::GetUpperScale());

    std::set<uint64_t> objects;
    auto process = [topSize, &objects, &processObject](uint64_t storedId) {
      if (objects.insert(storedId).second && objects.size() <= topSize)
        processObject(LocalityObject::FromStoredId(storedId));
    };

    for (auto const & i : intervals)
    {
      if (objects.size() >= topSize)
        return;
      m_intervalIndex->ForEach(process, i.first, i.second);
    }
  }

private:
  std::unique_ptr<IntervalIndex<Reader, uint64_t>> m_intervalIndex;
};
}  // namespace indexer
