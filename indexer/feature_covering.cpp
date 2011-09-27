#include "feature_covering.hpp"
#include "cell_coverer.hpp"
#include "cell_id.hpp"
#include "feature.hpp"
#include "scales.hpp"

#include "../geometry/covering_utils.hpp"

#include "../base/base.hpp"
#include "../base/stl_add.hpp"

#include "../std/algorithm.hpp"
#include "../std/bind.hpp"
#include "../std/vector.hpp"

namespace
{

// This class should only be used with covering::CoverObject()!
class FeatureIntersector
{
public:
  struct Trg
  {
    m2::PointD m_A, m_B, m_C;
    Trg(m2::PointD const & a, m2::PointD const & b, m2::PointD const & c)
      : m_A(a), m_B(b), m_C(c) {}
  };

  vector<m2::PointD> m_Polyline;
  vector<Trg> m_Trg;

  // Note:
  // 1. Here we don't need to differentiate between CELL_OBJECT_INTERSECT and OBJECT_INSIDE_CELL.
  // 2. We can return CELL_OBJECT_INTERSECT instead of CELL_INSIDE_OBJECT - it's just
  //    a performance penalty.
  covering::CellObjectIntersection operator() (RectId const & cell) const
  {
    for (size_t i = 0; i < m_Trg.size(); ++i)
    {
      covering::CellObjectIntersection const res =
          covering::IntersectCellWithTriangle(cell, m_Trg[i].m_A, m_Trg[i].m_B, m_Trg[i].m_C);
      switch (res)
      {
      case covering::CELL_OBJECT_NO_INTERSECTION:
        break;
      case covering::CELL_INSIDE_OBJECT:
        return covering::CELL_INSIDE_OBJECT;
      case covering::CELL_OBJECT_INTERSECT:
      case covering::OBJECT_INSIDE_CELL:
        return covering::CELL_OBJECT_INTERSECT;
      }
    }
    for (size_t i = 1; i < m_Polyline.size(); ++i)
    {
      covering::CellObjectIntersection const res =
          covering::IntersectCellWithLine(cell, m_Polyline[i], m_Polyline[i-1]);
      switch (res)
      {
      case covering::CELL_OBJECT_NO_INTERSECTION:
        break;
      case covering::CELL_INSIDE_OBJECT:
        ASSERT(false, (cell, i, m_Polyline));
        return covering::CELL_OBJECT_INTERSECT;
      case covering::CELL_OBJECT_INTERSECT:
      case covering::OBJECT_INSIDE_CELL:
        return covering::CELL_OBJECT_INTERSECT;
      }
    }
    return covering::CELL_OBJECT_NO_INTERSECTION;
  }

  typedef CellIdConverter<MercatorBounds, RectId> CellIdConverterType;

  static m2::PointD ConvertPoint(double x, double y)
  {
    return m2::PointD(CellIdConverterType::XToCellIdX(x),
                      CellIdConverterType::YToCellIdY(y));
  }

  void operator() (pair<double, double> const & pt)
  {
    m_Polyline.push_back(ConvertPoint(pt.first, pt.second));
  }

  void operator() (m2::PointD const & a, m2::PointD const & b, m2::PointD const & c)
  {
    m_Trg.push_back(Trg(ConvertPoint(a.x, a.y),
                        ConvertPoint(b.x, b.y),
                        ConvertPoint(c.x, c.y)));
  }
};

}

namespace covering
{

vector<int64_t> CoverFeature(FeatureType const & f, int cellDepth, uint64_t cellPenaltyArea)
{
  FeatureIntersector featureIntersector;
  f.ForEachPointRef(featureIntersector, -1);
  f.ForEachTriangleRef(featureIntersector, -1);

  CHECK(!featureIntersector.m_Trg.empty() || !featureIntersector.m_Polyline.empty(), \
        (f.DebugString(-1)));

  if (featureIntersector.m_Trg.empty() && featureIntersector.m_Polyline.size() == 1)
  {
    m2::PointD pt = featureIntersector.m_Polyline[0];
    return vector<int64_t>(
          1, RectId::FromXY(static_cast<uint32_t>(pt.x), static_cast<uint32_t>(pt.y),
                            RectId::DEPTH_LEVELS - 1).ToInt64(cellDepth));
  }

  vector<RectId> cells;
  covering::CoverObject(featureIntersector, cellPenaltyArea, cells, RectId::Root());

  vector<int64_t> res(cells.size());
  for (size_t i = 0; i < cells.size(); ++i)
    res[i] = cells[i].ToInt64(cellDepth);

  return res;
}

void SortAndMergeIntervals(IntervalsT v, IntervalsT & res)
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

IntervalsT SortAndMergeIntervals(IntervalsT const & v)
{
  IntervalsT res;
  SortAndMergeIntervals(v, res);
  return res;
}

void AppendLowerLevels(RectId id, int cellDepth, IntervalsT & intervals)
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

void CoverViewportAndAppendLowerLevels(m2::RectD const & r, int cellDepth, IntervalsT & res)
{
  vector<RectId> ids;
  CoverRect<MercatorBounds, RectId>(r.minX(), r.minY(), r.maxX(), r.maxY(), 8, cellDepth, ids);

  IntervalsT intervals;
  intervals.reserve(ids.size() * 4);

  for (size_t i = 0; i < ids.size(); ++i)
    AppendLowerLevels(ids[i], cellDepth, intervals);

  SortAndMergeIntervals(intervals, res);
}

RectId GetRectIdAsIs(m2::RectD const & r)
{
  double const eps = MercatorBounds::GetCellID2PointAbsEpsilon();

  return CellIdConverter<MercatorBounds, RectId>::Cover2PointsWithCell(
    MercatorBounds::ClampX(r.minX() + eps),
    MercatorBounds::ClampY(r.minY() + eps),
    MercatorBounds::ClampX(r.maxX() - eps),
    MercatorBounds::ClampY(r.maxY() - eps));
}

int GetCodingDepth(pair<int, int> const & scalesR)
{
  ASSERT_LESS_OR_EQUAL ( scalesR.first, scalesR.second, () );

  int const delta = scales::GetUpperScale() - scalesR.second;
  ASSERT_GREATER_OR_EQUAL ( delta, 0, () );

  return (RectId::DEPTH_LEVELS - delta);
}

IntervalsT const & CoveringGetter::Get(pair<int, int> const & scaleR)
{
  int const cellDepth = GetCodingDepth(scaleR);
  int const ind = (cellDepth == RectId::DEPTH_LEVELS ? 0 : 1);

  if (m_res[ind].empty())
  {
    switch (m_mode)
    {
    case 0:
      CoverViewportAndAppendLowerLevels(m_rect, cellDepth, m_res[ind]);
      break;

    case 1:
      AppendLowerLevels(GetRectIdAsIs(m_rect), cellDepth, m_res[ind]);
      break;

    case 2:
      m_res[ind].push_back(IntervalsT::value_type(0, static_cast<int64_t>((1ULL << 63) - 1)));
      break;
    }
  }

  return m_res[ind];
}

}
