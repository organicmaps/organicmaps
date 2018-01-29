#include "indexer/feature_covering.hpp"

#include "indexer/cell_coverer.hpp"
#include "indexer/cell_id.hpp"
#include "indexer/feature.hpp"
#include "indexer/locality_object.hpp"
#include "indexer/scales.hpp"

#include "geometry/covering_utils.hpp"

namespace
{

// This class should only be used with covering::CoverObject()!
class FeatureIntersector
{
public:
  struct Trg
  {
    m2::PointD m_a, m_b, m_c;
    Trg(m2::PointD const & a, m2::PointD const & b, m2::PointD const & c)
      : m_a(a), m_b(b), m_c(c) {}
  };

  vector<m2::PointD> m_polyline;
  vector<Trg> m_trg;
  m2::RectD m_rect;

  // Note:
  // 1. Here we don't need to differentiate between CELL_OBJECT_INTERSECT and OBJECT_INSIDE_CELL.
  // 2. We can return CELL_OBJECT_INTERSECT instead of CELL_INSIDE_OBJECT - it's just
  //    a performance penalty.
  covering::CellObjectIntersection operator() (RectId const & cell) const
  {
    using namespace covering;

    m2::RectD cellRect;
    {
      // Check for limit rect intersection.
      pair<uint32_t, uint32_t> const xy = cell.XY();
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

      CellObjectIntersection const res =
          IntersectCellWithTriangle(cell, m_trg[i].m_a, m_trg[i].m_b, m_trg[i].m_c);

      switch (res)
      {
      case CELL_OBJECT_NO_INTERSECTION:
        break;
      case CELL_INSIDE_OBJECT:
        return CELL_INSIDE_OBJECT;
      case CELL_OBJECT_INTERSECT:
      case OBJECT_INSIDE_CELL:
        return CELL_OBJECT_INTERSECT;
      }
    }

    for (size_t i = 1; i < m_polyline.size(); ++i)
    {
      CellObjectIntersection const res =
          IntersectCellWithLine(cell, m_polyline[i], m_polyline[i-1]);
      switch (res)
      {
      case CELL_OBJECT_NO_INTERSECTION:
        break;
      case CELL_INSIDE_OBJECT:
        ASSERT(false, (cell, i, m_polyline));
        return CELL_OBJECT_INTERSECT;
      case CELL_OBJECT_INTERSECT:
      case OBJECT_INSIDE_CELL:
        return CELL_OBJECT_INTERSECT;
      }
    }

    return CELL_OBJECT_NO_INTERSECTION;
  }

  typedef CellIdConverter<MercatorBounds, RectId> CellIdConverterType;

  m2::PointD ConvertPoint(m2::PointD const & p)
  {
    m2::PointD const pt(CellIdConverterType::XToCellIdX(p.x),
                        CellIdConverterType::YToCellIdY(p.y));
    m_rect.Add(pt);
    return pt;
  }

  void operator() (m2::PointD const & pt)
  {
    m_polyline.push_back(ConvertPoint(pt));
  }

  void operator() (m2::PointD const & a, m2::PointD const & b, m2::PointD const & c)
  {
    m_trg.emplace_back(ConvertPoint(a), ConvertPoint(b), ConvertPoint(c));
  }
};

void GetIntersection(FeatureType const & f, FeatureIntersector & fIsect)
{
  // We need to cover feature for the best geometry, because it's indexed once for the
  // first top level scale. Do reset current cached geometry first.
  f.ResetGeometry();
  int const scale = FeatureType::BEST_GEOMETRY;

  f.ForEachPoint(fIsect, scale);
  f.ForEachTriangle(fIsect, scale);

  CHECK(!(fIsect.m_trg.empty() && fIsect.m_polyline.empty()) &&
        f.GetLimitRect(scale).IsValid(), (f.DebugString(scale)));
}

vector<int64_t> CoverIntersection(FeatureIntersector const & fIsect, int cellDepth,
                                  uint64_t cellPenaltyArea)
{
  if (fIsect.m_trg.empty() && fIsect.m_polyline.size() == 1)
  {
    m2::PointD const pt = fIsect.m_polyline[0];
    return vector<int64_t>(
          1, RectId::FromXY(static_cast<uint32_t>(pt.x), static_cast<uint32_t>(pt.y),
                            RectId::DEPTH_LEVELS - 1).ToInt64(cellDepth));
  }

  vector<RectId> cells;
  covering::CoverObject(fIsect, cellPenaltyArea, cells, cellDepth, RectId::Root());

  vector<int64_t> res(cells.size());
  for (size_t i = 0; i < cells.size(); ++i)
    res[i] = cells[i].ToInt64(cellDepth);

  return res;
}
}

namespace covering
{
vector<int64_t> CoverFeature(FeatureType const & f, int cellDepth, uint64_t cellPenaltyArea)
{
  FeatureIntersector fIsect;
  GetIntersection(f, fIsect);
  return CoverIntersection(fIsect, cellDepth, cellPenaltyArea);
}

vector<int64_t> CoverLocality(indexer::LocalityObject const & o, int cellDepth,
                              uint64_t cellPenaltyArea)
{
  FeatureIntersector fIsect;
  o.ForEachPoint(fIsect);
  o.ForEachTriangle(fIsect);
  return CoverIntersection(fIsect, cellDepth, cellPenaltyArea);
}

void SortAndMergeIntervals(Intervals v, Intervals & res)
{
#ifdef DEBUG
  ASSERT ( res.empty(), () );
  for (size_t i = 0; i < v.size(); ++i)
    ASSERT_LESS(v[i].first, v[i].second, (i));
#endif

  sort(v.begin(), v.end());

  res.reserve(v.size());
  for (size_t i = 0; i < v.size(); ++i)
  {
    if (i == 0 || res.back().second < v[i].first)
      res.push_back(v[i]);
    else
      res.back().second = max(res.back().second, v[i].second);
  }
}

Intervals SortAndMergeIntervals(Intervals const & v)
{
  Intervals res;
  SortAndMergeIntervals(v, res);
  return res;
}

void AppendLowerLevels(RectId id, int cellDepth, Intervals & intervals)
{
  int64_t idInt64 = id.ToInt64(cellDepth);
  intervals.push_back(make_pair(idInt64, idInt64 + id.SubTreeSize(cellDepth)));
  while (id.Level() > 0)
  {
    id = id.Parent();
    idInt64 = id.ToInt64(cellDepth);
    intervals.push_back(make_pair(idInt64, idInt64 + 1));
  }
}

void CoverViewportAndAppendLowerLevels(m2::RectD const & r, int cellDepth, Intervals & res)
{
  vector<RectId> ids;
  ids.reserve(SPLIT_RECT_CELLS_COUNT);
  CoverRect<MercatorBounds, RectId>(r, SPLIT_RECT_CELLS_COUNT, cellDepth, ids);

  Intervals intervals;
  for (size_t i = 0; i < ids.size(); ++i)
    AppendLowerLevels(ids[i], cellDepth, intervals);

  SortAndMergeIntervals(intervals, res);
}

RectId GetRectIdAsIs(m2::RectD const & r)
{
  double const eps = MercatorBounds::GetCellID2PointAbsEpsilon();
  using TConverter = CellIdConverter<MercatorBounds, RectId>;

  return TConverter::Cover2PointsWithCell(
    MercatorBounds::ClampX(r.minX() + eps),
    MercatorBounds::ClampY(r.minY() + eps),
    MercatorBounds::ClampX(r.maxX() - eps),
    MercatorBounds::ClampY(r.maxY() - eps));
}

int GetCodingDepth(int scale)
{
  int const delta = scales::GetUpperScale() - scale;
  ASSERT_GREATER_OR_EQUAL ( delta, 0, () );

  return (RectId::DEPTH_LEVELS - delta);
}

Intervals const & CoveringGetter::Get(int scale)
{
  int const cellDepth = GetCodingDepth(scale);
  int const ind = (cellDepth == RectId::DEPTH_LEVELS ? 0 : 1);

  if (m_res[ind].empty())
  {
    switch (m_mode)
    {
    case ViewportWithLowLevels:
      CoverViewportAndAppendLowerLevels(m_rect, cellDepth, m_res[ind]);
      break;

    case LowLevelsOnly:
    {
      RectId id = GetRectIdAsIs(m_rect);
      while (id.Level() >= cellDepth)
        id = id.Parent();
      AppendLowerLevels(id, cellDepth, m_res[ind]);

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
      m_res[ind].push_back(Intervals::value_type(0, static_cast<int64_t>((1ULL << 63) - 1)));
      break;
    }
  }

  return m_res[ind];
}

}
