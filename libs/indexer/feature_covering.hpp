#pragma once

#include "indexer/cell_coverer.hpp"
#include "indexer/cell_id.hpp"
#include "indexer/scales.hpp"

#include "coding/point_coding.hpp"

#include "geometry/cellid.hpp"
#include "geometry/mercator.hpp"
#include "geometry/rect2d.hpp"

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

// Given a vector of intervals [a, b), sort them and merge overlapping intervals.
Intervals SortAndMergeIntervals(Intervals intervals);

template <int DEPTH_LEVELS>
m2::CellId<DEPTH_LEVELS> GetRectIdAsIs(m2::RectD const & r)
{
  double const eps = kMwmPointAccuracy;
  using Converter = CellIdConverter<mercator::Bounds, m2::CellId<DEPTH_LEVELS>>;

  return Converter::Cover2PointsWithCell(mercator::ClampX(r.minX() + eps), mercator::ClampY(r.minY() + eps),
                                         mercator::ClampX(r.maxX() - eps), mercator::ClampY(r.maxY() - eps));
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
void AppendLowerLevels(m2::CellId<DEPTH_LEVELS> id, int cellDepth, ToDo && toDo)
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
Intervals CoverViewportAndAppendLowerLevels(m2::RectD const & r, int cellDepth)
{
  std::vector<m2::CellId<DEPTH_LEVELS>> ids;
  ids.reserve(SPLIT_RECT_CELLS_COUNT);
  CoverRect<mercator::Bounds, m2::CellId<DEPTH_LEVELS>>(r, SPLIT_RECT_CELLS_COUNT, cellDepth - 1, ids);

  Intervals intervals;
  for (auto const & id : ids)
  {
    AppendLowerLevels<DEPTH_LEVELS>(id, cellDepth,
                                    [&intervals](Interval const & interval) { intervals.push_back(interval); });
  }

  return SortAndMergeIntervals(std::move(intervals));
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

  Intervals const & Get(int scale);
};

// Aggregates the covering of several rects into one merged set of cell-id intervals, built
// incrementally as rects are Add()-ed. Unlike CoveringGetter (a single rect, computed lazily), the
// coding |scale| is fixed at construction, so a geometry-index query reads only the cells near the
// added rects instead of one big bounding rect. Mirrors CoveringGetter's Get()/GetRect() interface,
// so the same MWM-reading path can consume either.
// Only ViewportWithLowLevels semantics are supported - the meaningful mode for multi-rect queries.
class AggCovering
{
public:
  // |scale| is the geometry coding scale at which the cell intervals are built - typically the
  // target MWM's last coding scale (scales::GetUpperScale() for country MWMs).
  /// @todo Pass the MWM type (World, WorldCoasts, Country).
  explicit AggCovering(int scale) : m_cellDepth(GetCodingDepth<RectId::DEPTH_LEVELS>(scale)) {}

  // Covers |rect| and appends its intervals to the accumulated set (sorted/merged lazily in Get()).
  void Add(m2::RectD const & rect);

  bool IsEmpty() const { return m_intervals.empty(); }

  // Bounding rect of all added rects; used to read edited features.
  m2::RectD const & GetRect() const { return m_unionRect; }

  // Kept for interface compatibility with CoveringGetter; |scale| is ignored - the intervals were
  // built at the construction scale. Sorts and merges the accumulated intervals on the first call.
  Intervals const & Get(int scale);

private:
  int m_cellDepth;
  m2::RectD m_unionRect;
  Intervals m_intervals;
  bool m_sorted = false;
};
}  // namespace covering
