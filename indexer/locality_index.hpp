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
#include <utility>

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
  using ProcessCloseObject = std::function<void(base::GeoObjectId const & objectId, double closenessWeight)>;

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
  // |processObject| gets object id in the first argument |objectId| and closeness weight
  // in the seсond argument |closenessWeight| (closeness weight in the range (0.0, 1.0]).
  void ForClosestToPoint(ProcessCloseObject const & processObject, m2::PointD const & center,
                         double radiusM, uint32_t sizeHint) const
  {
    using Converter = CellIdConverter<MercatorBounds, m2::CellId<DEPTH_LEVELS>>;

    auto const rect =
        MercatorBounds::RectByCenterXYAndSizeInMeters(center, radiusM);
    covering::CoveringGetter cov(rect, covering::CoveringMode::Spiral);
    covering::Intervals const & intervals = cov.Get<DEPTH_LEVELS>(scales::GetUpperScale());

    CHECK_EQUAL(intervals.begin()->first, intervals.begin()->second - 1, ());
    auto cellDepth = covering::GetCodingDepth<DEPTH_LEVELS>(scales::GetUpperScale());
    auto bestCell = m2::CellId<DEPTH_LEVELS>::FromInt64(intervals.begin()->first, cellDepth);
    std::set<int64_t> bestCells;
    while (bestCell.Level() > 0)
    {
      bestCells.insert(bestCell.ToInt64(cellDepth));
      bestCell = bestCell.Parent();
    }

    std::map<uint64_t, double> objectWeights{};

    auto const centralCell = Converter::ToCellId(center.x, center.y);
    auto const centralCellXY = centralCell.XY();

    auto chebyshevDistance = [centralCellXY] (auto && cellXY) {
      auto abs = [](auto && a, auto && b) { return a > b ? a - b : b - a; };
      auto const distanceX = abs(centralCellXY.first, cellXY.first);
      auto const distanceY = abs(centralCellXY.second, cellXY.second);
      return std::max(distanceX, distanceY);
    };

    auto cellRelativeWeight = [&] (int64_t cellNumber) {
      if (bestCells.find(cellNumber) != bestCells.end())
        return 1.0;

      auto const cell = m2::CellId<DEPTH_LEVELS>::FromInt64(cellNumber, cellDepth);
      auto const distance = chebyshevDistance(cell.XY());
      CHECK_GREATER(distance, 0, ());

      return 1.0 / distance;
    };

    auto processAll = [&] (int64_t cellNumber, uint64_t storedId) {
      auto const objectId = LocalityObject::FromStoredId(storedId).GetEncodedId();
      auto & objectWeight = objectWeights[objectId];
      objectWeight = max(objectWeight, cellRelativeWeight(cellNumber));
    };

    auto process = [&](int64_t cellNumber, uint64_t storedId) {
      if (objectWeights.size() < sizeHint)
        processAll(cellNumber, storedId);
    };

    for (auto const & i : intervals)
    {
      if (bestCells.find(i.first) != bestCells.end())
      {
        m_intervalIndex->ForEach(processAll, i.first, i.second);
      }
      else
      {
        m_intervalIndex->ForEach(process, i.first, i.second);
        if (objectWeights.size() >= sizeHint)
          return;
      }
    }

    std::vector<std::pair<uint64_t, double>> result(objectWeights.begin(), objectWeights.end());
    std::sort(result.begin(), result.end(), [] (auto && l, auto && r) { return l.second > r.second;});
    for (auto const & object : result)
      processObject(base::GeoObjectId(object.first), object.second);
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
