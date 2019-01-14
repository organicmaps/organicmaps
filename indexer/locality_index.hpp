#pragma once

#include "indexer/cell_id.hpp"
#include "indexer/feature_covering.hpp"
#include "indexer/interval_index.hpp"
#include "indexer/locality_object.hpp"
#include "indexer/scales.hpp"

#include "coding/file_container.hpp"

#include "geometry/rect2d.hpp"

#include "base/geo_object_id.hpp"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>

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
  using ProcessClosestObject = std::function<void(base::GeoObjectId const &, double distance)>;

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
  void ForClosestToPoint(ProcessClosestObject const & processObject, m2::PointD const & center,
                         double radiusM, uint32_t sizeHint) const
  {
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

    struct ResultItem
    {
      uint64_t m_ObjectId;
      double m_Distance;
    };
    using namespace boost::multi_index;
    auto result = multi_index_container<ResultItem,
                      indexed_by<hashed_unique<member<ResultItem, uint64_t, &ResultItem::m_ObjectId>>,
                                 ordered_non_unique<member<ResultItem, double, &ResultItem::m_Distance>>>>{};

    auto processAll = [&result, &bestCells, cellDepth, &center](int64_t cellNumber, uint64_t storedId) {
      auto distance = 0.0;
      if (bestCells.find(cellNumber) == bestCells.end())
      {
        using Converter = CellIdConverter<MercatorBounds, m2::CellId<DEPTH_LEVELS>>;

        auto cell = m2::CellId<DEPTH_LEVELS>::FromInt64(cellNumber, cellDepth);
        auto cellCenter = Converter::FromCellId(cell);
        distance = MercatorBounds::DistanceOnEarth(center, cellCenter);
      }

      auto objectId = LocalityObject::FromStoredId(storedId).GetEncodedId();
      auto resultInsert = result.insert({objectId, distance});
      if (resultInsert.second)
        return;
      auto const & object = *resultInsert.first;
      if (distance < object.m_Distance)
        result.replace(resultInsert.first, {objectId, distance});
    };

    auto process = [&](int64_t cellNumber, uint64_t storedId) {
      if (result.size() < sizeHint)
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
        if (result.size() >= sizeHint)
          return;
      }
    }

    for (auto const & object : result.template get<1>())
      processObject(base::GeoObjectId(object.m_ObjectId), object.m_Distance);
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
