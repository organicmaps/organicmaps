#include "covering.hpp"
#include "cell_coverer.hpp"
#include "cell_id.hpp"
#include "feature.hpp"
#include "../geometry/covering.hpp"
#include "../base/base.hpp"
#include "../base/stl_add.hpp"
#include "../std/algorithm.hpp"
#include "../std/bind.hpp"

namespace
{

class FeatureIntersector
{
public:
  struct Trg
  {
    m2::PointD m_A, m_B, m_C;
    Trg(m2::PointU a, m2::PointU b, m2::PointU c) : m_A(a), m_B(b), m_C(c) {}
  };
  vector<m2::PointD> m_Polyline;
  vector<Trg> m_Trg;

  covering::CellObjectIntersection operator() (RectId const & cell) const
  {
    for (size_t i = 1; i < m_Polyline.size(); ++i)
      if (covering::IntersectCellWithLine(cell, m_Polyline[i], m_Polyline[i-1]))
        return covering::CELL_OBJECT_INTERSECT;
    for (size_t i = 0; i < m_Trg.size(); ++i)
      if (covering::CellObjectIntersection res =
          covering::IntersectCellWithTriangle(cell, m_Trg[i].m_A, m_Trg[i].m_B, m_Trg[i].m_C))
        return res == covering::OBJECT_INSIDE_CELL ? covering::CELL_OBJECT_INTERSECT : res;
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

vector<int64_t> covering::CoverFeature(FeatureType const & f,
                                       uint64_t cellPenaltyAreaPerNode)
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
          1, RectId::FromXY(static_cast<uint32_t>(pt.x), static_cast<uint32_t>(pt.y)).ToInt64());
  }

  vector<RectId> cells;
  covering::CoverObject(featureIntersector, cellPenaltyAreaPerNode, cells);

  vector<int64_t> res(cells.size());
  for (size_t i = 0; i < cells.size(); ++i)
    res[i] = cells[i].ToInt64();

  return res;
}

vector<pair<int64_t, int64_t> > covering::SortAndMergeIntervals(vector<pair<int64_t, int64_t> > v)
{
#ifdef DEBUG
  for (size_t i = 0; i < v.size(); ++i)
    ASSERT_LESS(v[i].first, v[i].second, (i));
#endif
  sort(v.begin(), v.end());
  vector<pair<int64_t, int64_t> > res;
  res.reserve(v.size());
  for (size_t i = 0; i < v.size(); ++i)
  {
    if (i == 0 || res.back().second < v[i].first)
      res.push_back(v[i]);
    else
      res.back().second = max(res.back().second, v[i].second);
  }
  return res;
}

vector<pair<int64_t, int64_t> > covering::CoverViewportAndAppendLowerLevels(m2::RectD const & rect)
{
  vector<RectId> ids;
  CoverRect<MercatorBounds, RectId>(rect.minX(), rect.minY(), rect.maxX(), rect.maxY(), 8, ids);
  vector<pair<int64_t, int64_t> > intervals;
  intervals.reserve(ids.size() * 4);
  for (vector<RectId>::const_iterator it = ids.begin(); it != ids.end(); ++it)
  {
    RectId id = *it;
    int64_t idInt64 = id.ToInt64();
    intervals.push_back(pair<int64_t, int64_t>(idInt64, idInt64 + id.SubTreeSize()));
    while (id.Level() > 0)
    {
      id = id.Parent();
      idInt64 = id.ToInt64();
      intervals.push_back(pair<int64_t, int64_t>(idInt64, idInt64 + 1));
    }
  }
  return SortAndMergeIntervals(intervals);
}
