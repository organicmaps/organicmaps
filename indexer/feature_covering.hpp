#pragma once

#include "indexer/cell_coverer.hpp"
#include "indexer/cell_id.hpp"
#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"
#include "geometry/rect2d.hpp"

#include "coding/point_to_integer.hpp"

#include "base/logging.hpp"

#include <cstdint>
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
std::vector<int64_t> CoverFeature(FeatureType const & feature, int cellDepth,
                                  uint64_t cellPenaltyArea);

std::vector<int64_t> CoverLocality(indexer::LocalityObject const & o, int cellDepth,
                                   uint64_t cellPenaltyArea);

// Given a vector of intervals [a, b), sort them and merge overlapping intervals.
Intervals SortAndMergeIntervals(Intervals const & intervals);
void SortAndMergeIntervals(Intervals v, Intervals & res);

template <int DEPTH_LEVELS>
m2::CellId<DEPTH_LEVELS> GetRectIdAsIs(m2::RectD const & r)
{
  double const eps = MercatorBounds::GetCellID2PointAbsEpsilon();
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

template <int DEPTH_LEVELS>
void AppendLowerLevels(m2::CellId<DEPTH_LEVELS> id, int cellDepth, Intervals & intervals)
{
  int64_t idInt64 = id.ToInt64(cellDepth);
  intervals.push_back(std::make_pair(idInt64, idInt64 + id.SubTreeSize(cellDepth)));
  while (id.Level() > 0)
  {
    id = id.Parent();
    idInt64 = id.ToInt64(cellDepth);
    intervals.push_back(make_pair(idInt64, idInt64 + 1));
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
    AppendLowerLevels<DEPTH_LEVELS>(id, cellDepth, intervals);

  SortAndMergeIntervals(intervals, res);
}

enum CoveringMode
{
  ViewportWithLowLevels = 0,
  LowLevelsOnly,
  FullCover
};

class CoveringGetter
{
  Intervals m_res[2];

  m2::RectD const & m_rect;
  CoveringMode m_mode;

public:
  CoveringGetter(m2::RectD const & r, CoveringMode mode) : m_rect(r), m_mode(mode) {}

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
        AppendLowerLevels<DEPTH_LEVELS>(id, cellDepth, m_res[ind]);

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
      }
    }

    return m_res[ind];
  }
};
}
