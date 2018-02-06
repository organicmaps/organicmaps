#pragma once

#include "indexer/cell_id.hpp"
#include "indexer/feature_covering.hpp"
#include "indexer/interval_index.hpp"
#include "indexer/locality_object.hpp"
#include "indexer/scales.hpp"

#include "geometry/rect2d.hpp"

#include "base/osm_id.hpp"

#include <functional>
#include <memory>

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
          [&processObject](uint64_t stored_id) {
            processObject(LocalityObject::FromStoredId(stored_id));
          },
          i.first, i.second);
    }
  }

private:
  std::unique_ptr<IntervalIndex<Reader, uint64_t>> m_intervalIndex;
};
}  // namespace indexer
