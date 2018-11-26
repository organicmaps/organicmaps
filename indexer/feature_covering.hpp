#pragma once

#include "indexer/cell_coverer.hpp"
#include "indexer/cell_id.hpp"
#include "indexer/scales.hpp"

#include "coding/point_coding.hpp"

#include "geometry/cellid.hpp"
#include "geometry/mercator.hpp"
#include "geometry/rect2d.hpp"

#include "base/logging.hpp"

#include <cstdint>
#include <set>
#include <utility>
#include <vector>

class FeatureType;

namespace indexer
{
class LocalityObject;
}  // namespace indexer

namespace covering
{
typedef std::pair<int64_t, int64_t> Interval;
typedef std::vector<Interval> Intervals;

// Cover feature with RectIds and return their integer representations.
std::vector<int64_t> CoverFeature(FeatureType & feature, int cellDepth, uint64_t cellPenaltyArea);

std::vector<int64_t> CoverGeoObject(indexer::LocalityObject const & o, int cellDepth);

std::vector<int64_t> CoverRegion(indexer::LocalityObject const & o, int cellDepth);

// Given a vector of intervals [a, b), sort them and merge overlapping intervals.
Intervals SortAndMergeIntervals(Intervals const & intervals);
void SortAndMergeIntervals(Intervals v, Intervals & res);

template <int DEPTH_LEVELS>
m2::CellId<DEPTH_LEVELS> GetRectIdAsIs(m2::RectD const & r)
{
  double const eps = kMwmPointAccuracy;
  using Converter = CellIdConverter<MercatorBounds, m2::CellId<DEPTH_LEVELS>>;

  return Converter::Cover2PointsWithCell(
      MercatorBounds::ClampX(r.minX() + eps), MercatorBounds::ClampY(r.minY() + eps),
      MercatorBounds::ClampX(r.maxX() - eps), MercatorBounds::ClampY(r.maxY() - eps));
}

// Calculate cell coding depth according to max visual scale for mwm.
template <int DEPTH_LEVELS>
int GetCodingDepth(int scale)
{
  int const delta = scales::GetUpperScale() - scale;
  ASSERT_GREATER_OR_EQUAL(delta, 0, ());
  return DEPTH_LEVELS - delta;
}

template <int DEPTH_LEVELS, typename ToDo>
void AppendLowerLevels(m2::CellId<DEPTH_LEVELS> id, int cellDepth, ToDo const & toDo)
{
  int64_t idInt64 = id.ToInt64(cellDepth);
  toDo(std::make_pair(idInt64, idInt64 + id.SubTreeSize(cellDepth)));
  while (id.Level() > 0)
  {
    id = id.Parent();
    idInt64 = id.ToInt64(cellDepth);
    toDo(std::make_pair(idInt64, idInt64 + 1));
  }
}

template <int DEPTH_LEVELS>
void CoverViewportAndAppendLowerLevels(m2::RectD const & r, int cellDepth, Intervals & res)
{
  std::vector<m2::CellId<DEPTH_LEVELS>> ids;
  ids.reserve(SPLIT_RECT_CELLS_COUNT);
  CoverRect<MercatorBounds, m2::CellId<DEPTH_LEVELS>>(r, SPLIT_RECT_CELLS_COUNT, cellDepth, ids);

  Intervals intervals;
  for (auto const & id : ids)
  {
    AppendLowerLevels<DEPTH_LEVELS>(
        id, cellDepth, [&intervals](Interval const & interval) { intervals.push_back(interval); });
  }

  SortAndMergeIntervals(intervals, res);
}

enum CoveringMode
{
  ViewportWithLowLevels = 0,
  LowLevelsOnly,
  FullCover,
  Spiral
};

class CoveringGetter
{
  Intervals m_res[2];

  m2::RectD const & m_rect;
  CoveringMode m_mode;

public:
  CoveringGetter(m2::RectD const & r, CoveringMode mode) : m_rect(r), m_mode(mode) {}

  m2::RectD const & GetRect() const { return m_rect; }

  template <int DEPTH_LEVELS>
  Intervals const & Get(int scale)
  {
    int const cellDepth = GetCodingDepth<DEPTH_LEVELS>(scale);
    int const ind = (cellDepth == DEPTH_LEVELS ? 0 : 1);

    if (m_res[ind].empty())
    {
      switch (m_mode)
      {
      case ViewportWithLowLevels:
        CoverViewportAndAppendLowerLevels<DEPTH_LEVELS>(m_rect, cellDepth, m_res[ind]);
        break;

      case LowLevelsOnly:
      {
        m2::CellId<DEPTH_LEVELS> id = GetRectIdAsIs<DEPTH_LEVELS>(m_rect);
        while (id.Level() >= cellDepth)
          id = id.Parent();
        AppendLowerLevels<DEPTH_LEVELS>(id, cellDepth, [this, ind](Interval const & interval) {
          m_res[ind].push_back(interval);
        });

        // Check for optimal result intervals.
#if 0
        size_t oldSize = m_res[ind].size();
        Intervals res;
        SortAndMergeIntervals(m_res[ind], res);
        if (res.size() != oldSize)
          LOG(LINFO, ("Old =", oldSize, "; New =", res.size()));
        res.swap(m_res[ind]);
#endif
        break;
      }

      case FullCover:
        m_res[ind].push_back(
            Intervals::value_type(0, static_cast<int64_t>((uint64_t{1} << 63) - 1)));
        break;

      case Spiral:
      {
        std::vector<m2::CellId<DEPTH_LEVELS>> ids;
        CoverSpiral<MercatorBounds, m2::CellId<DEPTH_LEVELS>>(m_rect, cellDepth, ids);

        std::set<Interval> uniqueIds;
        auto insertInterval = [this, ind, &uniqueIds](Interval const & interval) {
          if (uniqueIds.insert(interval).second)
            m_res[ind].push_back(interval);
        };

        for (auto const & id : ids)
        {
          if (cellDepth > id.Level())
            AppendLowerLevels<DEPTH_LEVELS>(id, cellDepth, insertInterval);
        }
      }
      }
    }

    return m_res[ind];
  }
};
}
