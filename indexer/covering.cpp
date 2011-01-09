#include "covering.hpp"
#include "cell_coverer.hpp"
#include "cell_id.hpp"
#include "feature.hpp"
#include "../geometry/covering.hpp"
#include "../base/base.hpp"
#include "../base/stl_add.hpp"
#include "../std/algorithm.hpp"
#include "../std/bind.hpp"

// TODO: Redo polyline covering right.

namespace
{
  template <class BoundsT, class CoveringT>
  class TriangleCoverer
  {
  public:
    typedef typename CoveringT::CellId CellId;
    typedef CellIdConverter<BoundsT, CellId> CellIdConverterType;

    explicit TriangleCoverer(CoveringT const & covering) : m_Covering(1, covering) {}

    void operator () (m2::PointD const & a, m2::PointD const & b, m2::PointD const & c)
    {
      AddTriangle(a, b, c);
    }

    void AddTriangle(m2::PointD const & a, m2::PointD const & b, m2::PointD const & c)
    {
      m_Covering.push_back(CoveringT(
          m2::PointD(CellIdConverterType::XToCellIdX(a.x), CellIdConverterType::YToCellIdY(a.y)),
          m2::PointD(CellIdConverterType::XToCellIdX(b.x), CellIdConverterType::YToCellIdY(b.y)),
          m2::PointD(CellIdConverterType::XToCellIdX(c.x), CellIdConverterType::YToCellIdY(c.y))));
      while (m_Covering.size() > 1 &&
             m_Covering[m_Covering.size() - 2].Size() < m_Covering.back().Size())
      {
        m_Covering[m_Covering.size() - 2].Append(m_Covering.back());
        m_Covering.pop_back();
      }
    }

    CoveringT const & GetCovering()
    {
      ASSERT(!m_Covering.empty(), ());
#ifdef DEBUG
      // Make appends in another order and assert that result is the same.
      CoveringT dbgCovering(m_Covering[0]);
      for (size_t i = 1; i < m_Covering.size(); ++i)
        dbgCovering.Append(m_Covering[i]);
#endif
      while (m_Covering.size() > 1)
      {
        m_Covering[m_Covering.size() - 2].Append(m_Covering.back());
        m_Covering.pop_back();
      }
#ifdef DEBUG
      vector<CellId> dbgIds, ids;
      dbgCovering.OutputToVector(dbgIds);
      m_Covering[0].OutputToVector(ids);
      ASSERT_EQUAL(dbgIds, ids, ());
#endif
      return m_Covering[0];
    }

  private:
    vector<CoveringT> m_Covering;
  };
}

vector<int64_t> covering::CoverFeature(FeatureType const & f, int level)
{
  vector<CoordPointT> geometry;
  f.ForEachPoint(MakeBackInsertFunctor(geometry), level);

  vector<RectId> ids;
  if (!geometry.empty())
  {
    if (geometry.size() > 1)
    {
      // TODO: Tweak CoverPolyLine() depth level.
      CoverPolyLine<MercatorBounds, RectId>(geometry, RectId::DEPTH_LEVELS - 1, ids);
    }
    else
      ids.push_back(CoverPoint<MercatorBounds, RectId>(geometry[0]));
  }

  typedef covering::Covering<RectId> CoveringType;
  typedef TriangleCoverer<MercatorBounds, CoveringType> CovererType;
  CovererType coverer = CovererType(CoveringType(ids));
  f.ForEachTriangleRef(coverer, level);

  vector<int64_t> res;
  coverer.GetCovering().OutputToVector(res);
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
