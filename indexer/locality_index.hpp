#pragma once

#include "indexer/cell_id.hpp"
#include "indexer/feature_covering.hpp"
#include "indexer/interval_index.hpp"
#include "indexer/locality_object.hpp"
#include "indexer/scales.hpp"

#include "coding/file_container.hpp"

#include "geometry/rect2d.hpp"

#include "base/geo_object_id.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <set>
#include <string>

#include "defines.hpp"

namespace indexer
{
// Geometry index which stores base::GeoObjectId as object identifier.
// Used for geocoder server, stores only POIs and buildings which have address information.
// Based on IntervalIndex.
template <typename Reader, int DEPTH_LEVELS>
class LocalityIndex
{
public:
  using ProcessObject = std::function<void(base::GeoObjectId const &)>;

  LocalityIndex() = default;
  explicit LocalityIndex(Reader const & reader)
  {
    m_intervalIndex = std::make_unique<IntervalIndex<Reader, uint64_t>>(reader);
  }

  void ForEachInRect(ProcessObject const & processObject, m2::RectD const & rect) const
  {
    covering::CoveringGetter cov(rect, covering::CoveringMode::ViewportWithLowLevels);
    covering::Intervals const & intervals = cov.Get<DEPTH_LEVELS>(scales::GetUpperScale());

    for (auto const & i : intervals)
    {
      m_intervalIndex->ForEach(
            [&processObject](uint64_t storedId) {
        processObject(LocalityObject::FromStoredId(storedId));
      },
      i.first, i.second);
    }
  }

  // Applies |processObject| to the objects located within |radiusM| meters from |center|.
  // Application to the closest objects and only to them is not guaranteed and the order
  // of the objects is not specified.
  // However, the method attempts to process objects that are closer to |center| first
  // and stop after |sizeHint| objects have been processed. For stability, if an object from
  // an index cell has been processed, all other objects from this cell will be processed too,
  // thus probably overflowing the |sizeHint| limit.
  void ForClosestToPoint(ProcessObject const & processObject, m2::PointD const & center,
                         double radiusM, uint32_t sizeHint) const
  {
    auto const rect =
        MercatorBounds::RectByCenterXYAndSizeInMeters(center, radiusM);
    covering::CoveringGetter cov(rect, covering::CoveringMode::Spiral);
    covering::Intervals const & intervals = cov.Get<DEPTH_LEVELS>(scales::GetUpperScale());

    std::set<uint64_t> objects;
    auto processAll = [&objects, &processObject](uint64_t storedId) {
      if (objects.insert(storedId).second)
        processObject(LocalityObject::FromStoredId(storedId));
    };

    auto process = [&](uint64_t storedId) {
      if (objects.size() < sizeHint)
        processAll(storedId);
    };

    CHECK_EQUAL(intervals.begin()->first, intervals.begin()->second - 1, ());
    auto cellDepth = covering::GetCodingDepth<DEPTH_LEVELS>(scales::GetUpperScale());
    auto bestCell = m2::CellId<DEPTH_LEVELS>::FromInt64(intervals.begin()->first, cellDepth);
    std::set<int64_t> bestCells;
    while (bestCell.Level() > 0)
    {
      bestCells.insert(bestCell.ToInt64(cellDepth));
      bestCell = bestCell.Parent();
      --cellDepth;
    }

    for (auto const & i : intervals)
    {
      if (bestCells.find(i.first) != bestCells.end())
      {
        m_intervalIndex->ForEach(processAll, i.first, i.second);
      }
      else
      {
        m_intervalIndex->ForEach(process, i.first, i.second);
        if (objects.size() >= sizeHint)
          return;
      }
    }
  }

private:
  std::unique_ptr<IntervalIndex<Reader, uint64_t>> m_intervalIndex;
};

template <typename Reader>
using GeoObjectsIndex = LocalityIndex<Reader, kGeoObjectsDepthLevels>;

template <typename Reader>
using RegionsIndex = LocalityIndex<Reader, kRegionsDepthLevels>;

template <typename Reader>
struct GeoObjectsIndexBox
{
  static constexpr const char * kFileTag = GEO_OBJECTS_INDEX_FILE_TAG;

  using ReaderType = Reader;
  using IndexType = GeoObjectsIndex<ReaderType>;
};

template <typename Reader>
struct RegionsIndexBox
{
  static constexpr const char * kFileTag = REGIONS_INDEX_FILE_TAG;

  using ReaderType = Reader;
  using IndexType = RegionsIndex<ReaderType>;
};

template <typename IndexBox, typename Reader>
typename IndexBox::IndexType ReadIndex(std::string const & pathIndx)
{
  FilesContainerR cont(pathIndx);
  auto const offsetSize = cont.GetAbsoluteOffsetAndSize(IndexBox::kFileTag);
  Reader reader(pathIndx);
  typename IndexBox::ReaderType subReader(reader.CreateSubReader(offsetSize.first, offsetSize.second));
  typename IndexBox::IndexType index(subReader);
  return index;
}
}  // namespace indexer
