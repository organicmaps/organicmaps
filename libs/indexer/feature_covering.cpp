#include "indexer/feature_covering.hpp"

#include "indexer/feature.hpp"

#include "geometry/covering_utils.hpp"

#include <set>
#include <utility>
#include <vector>

namespace
{
// This class should only be used with covering::CoverObject()!
template <int DEPTH_LEVELS>
class FeatureIntersector
{
public:
  struct Trg
  {
    m2::PointD m_a, m_b, m_c;
    Trg(m2::PointD const & a, m2::PointD const & b, m2::PointD const & c) : m_a(a), m_b(b), m_c(c) {}
  };

  std::vector<m2::PointD> m_polyline;
  std::vector<Trg> m_trg;
  m2::RectD m_rect;

  // Note:
  // 1. Here we don't need to differentiate between CELL_OBJECT_INTERSECT and OBJECT_INSIDE_CELL.
  // 2. We can return CELL_OBJECT_INTERSECT instead of CELL_INSIDE_OBJECT - it's just
  //    a performance penalty.
  covering::CellObjectIntersection operator()(m2::CellId<DEPTH_LEVELS> const & cell) const
  {
    using namespace covering;

    m2::RectD cellRect;
    {
      // Check for limit rect intersection.
      std::pair<uint32_t, uint32_t> const xy = cell.XY();
      uint32_t const r = cell.Radius();
      ASSERT_GREATER_OR_EQUAL(xy.first, r, ());
      ASSERT_GREATER_OR_EQUAL(xy.second, r, ());

      cellRect = m2::RectD(xy.first - r, xy.second - r, xy.first + r, xy.second + r);
      if (!cellRect.IsIntersect(m_rect))
        return CELL_OBJECT_NO_INTERSECTION;
    }

    for (size_t i = 0; i < m_trg.size(); ++i)
    {
      m2::RectD r;
      r.Add(m_trg[i].m_a);
      r.Add(m_trg[i].m_b);
      r.Add(m_trg[i].m_c);
      if (!cellRect.IsIntersect(r))
        continue;

      CellObjectIntersection const res = IntersectCellWithTriangle(cell, m_trg[i].m_a, m_trg[i].m_b, m_trg[i].m_c);

      switch (res)
      {
      case CELL_OBJECT_NO_INTERSECTION: break;
      case CELL_INSIDE_OBJECT: return CELL_INSIDE_OBJECT;
      case CELL_OBJECT_INTERSECT:
      case OBJECT_INSIDE_CELL: return CELL_OBJECT_INTERSECT;
      }
    }

    for (size_t i = 1; i < m_polyline.size(); ++i)
    {
      CellObjectIntersection const res = IntersectCellWithLine(cell, m_polyline[i], m_polyline[i - 1]);
      switch (res)
      {
      case CELL_OBJECT_NO_INTERSECTION: break;
      case CELL_INSIDE_OBJECT: ASSERT(false, (cell, i, m_polyline)); return CELL_OBJECT_INTERSECT;
      case CELL_OBJECT_INTERSECT:
      case OBJECT_INSIDE_CELL: return CELL_OBJECT_INTERSECT;
      }
    }

    return CELL_OBJECT_NO_INTERSECTION;
  }

  using Converter = CellIdConverter<mercator::Bounds, m2::CellId<DEPTH_LEVELS>>;

  m2::PointD ConvertPoint(m2::PointD const & p)
  {
    m2::PointD const pt(Converter::XToCellIdX(p.x), Converter::YToCellIdY(p.y));
    m_rect.Add(pt);
    return pt;
  }

  void operator()(m2::PointD const & pt) { m_polyline.push_back(ConvertPoint(pt)); }

  void operator()(m2::PointD const & a, m2::PointD const & b, m2::PointD const & c)
  {
    m_trg.emplace_back(ConvertPoint(a), ConvertPoint(b), ConvertPoint(c));
  }
};

template <int DEPTH_LEVELS>
void GetIntersection(FeatureType & f, FeatureIntersector<DEPTH_LEVELS> & fIsect)
{
  // We need to cover feature for the best geometry, because it's indexed once for the
  // first top level scale. Do reset current cached geometry first.
  f.ResetGeometry();
  int const scale = FeatureType::BEST_GEOMETRY;

  f.ForEachPoint(fIsect, scale);
  f.ForEachTriangle(fIsect, scale);

  CHECK(!(fIsect.m_trg.empty() && fIsect.m_polyline.empty()) && f.GetLimitRect(scale).IsValid(), (f.DebugString()));
}

template <int DEPTH_LEVELS>
std::vector<int64_t> CoverIntersection(FeatureIntersector<DEPTH_LEVELS> const & fIsect, int cellDepth,
                                       uint64_t cellPenaltyArea)
{
  if (fIsect.m_trg.empty() && fIsect.m_polyline.size() == 1)
  {
    m2::PointD const pt = fIsect.m_polyline[0];
    return std::vector<int64_t>(
        1, m2::CellId<DEPTH_LEVELS>::FromXY(static_cast<uint32_t>(pt.x), static_cast<uint32_t>(pt.y), DEPTH_LEVELS - 1)
               .ToInt64(cellDepth));
  }

  std::vector<m2::CellId<DEPTH_LEVELS>> cells;
  covering::CoverObject(fIsect, cellPenaltyArea, cells, cellDepth, m2::CellId<DEPTH_LEVELS>::Root());

  std::vector<int64_t> res(cells.size());
  for (size_t i = 0; i < cells.size(); ++i)
    res[i] = cells[i].ToInt64(cellDepth);

  return res;
}
}  // namespace

namespace covering
{
std::vector<int64_t> CoverFeature(FeatureType & f, int cellDepth, uint64_t cellPenaltyArea)
{
  FeatureIntersector<RectId::DEPTH_LEVELS> fIsect;
  GetIntersection(f, fIsect);
  return CoverIntersection(fIsect, cellDepth, cellPenaltyArea);
}

Intervals SortAndMergeIntervals(Intervals v)
{
#ifdef DEBUG
  for (size_t i = 0; i < v.size(); ++i)
    ASSERT_LESS(v[i].first, v[i].second, (i));
#endif

  std::sort(v.begin(), v.end());

  Intervals res;
  res.reserve(v.size());
  for (size_t i = 0; i < v.size(); ++i)
    if (i == 0 || res.back().second < v[i].first)
      res.push_back(v[i]);
    else
      res.back().second = std::max(res.back().second, v[i].second);

  return res;
}

Intervals const & CoveringGetter::Get(int scale)
{
  int const cellDepth = GetCodingDepth<RectId::DEPTH_LEVELS>(scale);
  int const ind = (cellDepth == RectId::DEPTH_LEVELS ? 0 : 1);

  if (m_res[ind].empty())
  {
    switch (m_mode)
    {
    case ViewportWithLowLevels:
      m_res[ind] = CoverViewportAndAppendLowerLevels<RectId::DEPTH_LEVELS>(m_rect, cellDepth);
      break;

    case LowLevelsOnly:
    {
      m2::CellId<RectId::DEPTH_LEVELS> id = GetRectIdAsIs<RectId::DEPTH_LEVELS>(m_rect);
      while (id.Level() >= cellDepth)
        id = id.Parent();
      AppendLowerLevels<RectId::DEPTH_LEVELS>(
          id, cellDepth, [this, ind](Interval const & interval) { m_res[ind].push_back(interval); });
      break;
    }

    case FullCover:
      m_res[ind].push_back(Intervals::value_type(0, static_cast<int64_t>((uint64_t{1} << 63) - 1)));
      break;

    case Spiral:
    {
      std::vector<m2::CellId<RectId::DEPTH_LEVELS>> ids;
      CoverSpiral<mercator::Bounds, m2::CellId<RectId::DEPTH_LEVELS>>(m_rect, cellDepth - 1, ids);

      std::set<Interval> uniqueIds;
      auto insertInterval = [this, ind, &uniqueIds](Interval const & interval)
      {
        if (uniqueIds.insert(interval).second)
          m_res[ind].push_back(interval);
      };

      for (auto const & id : ids)
        if (cellDepth > id.Level())
          AppendLowerLevels<RectId::DEPTH_LEVELS>(id, cellDepth, insertInterval);
    }
    }
  }

  return m_res[ind];
}

void Covering::Add(m2::RectD const & rect)
{
  ASSERT(rect.IsValid(), ());
  m_unionRect.Add(rect);

  // Append this rect's covering; the whole set is sorted and merged once, lazily, in Get().
  auto cur = CoverViewportAndAppendLowerLevels<RectId::DEPTH_LEVELS>(rect, m_cellDepth);
  m_intervals.insert(m_intervals.end(), cur.begin(), cur.end());
  m_sorted = false;
}

Intervals const & Covering::Get(int /* scale */)
{
  if (!m_sorted)
  {
    // Merge intervals coming from all added rects (nearby rects share most parent-cell intervals).
    m_intervals = SortAndMergeIntervals(std::move(m_intervals));
    m_sorted = true;
  }
  return m_intervals;
}
}  // namespace covering
