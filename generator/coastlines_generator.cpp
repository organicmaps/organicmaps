#include "generator/coastlines_generator.hpp"
#include "generator/feature_builder.hpp"

#include "indexer/point_to_int64.hpp"

#include "geometry/region2d/binary_operators.hpp"

#include "base/string_utils.hpp"
#include "base/logging.hpp"

#include "3party/gflags/src/gflags/gflags.h"

#include "std/bind.hpp"


typedef m2::RegionI RegionT;
typedef m2::PointI PointT;
typedef m2::RectI RectT;

DECLARE_bool(fail_on_coasts);

CoastlineFeaturesGenerator::CoastlineFeaturesGenerator(uint32_t coastType,
                                                       int lowLevel, int highLevel, int maxPoints)
  : m_merger(POINT_COORD_BITS), m_coastType(coastType),
    m_lowLevel(lowLevel), m_highLevel(highLevel), m_maxPoints(maxPoints)
{
  ASSERT_LESS_OR_EQUAL ( m_lowLevel, m_highLevel, () );
}

namespace
{
  m2::RectD GetLimitRect(RegionT const & rgn)
  {
    RectT r = rgn.GetRect();
    return m2::RectD(r.minX(), r.minY(), r.maxX(), r.maxY());
  }

  inline PointT D2I(m2::PointD const & p)
  {
    m2::PointU const pu = PointD2PointU(p, POINT_COORD_BITS);
    return PointT(static_cast<int32_t>(pu.x), static_cast<int32_t>(pu.y));
  }

  template <class TreeT> class DoCreateRegion
  {
    TreeT & m_tree;

    RegionT m_rgn;
    m2::PointD m_pt;
    bool m_exist;

  public:
    DoCreateRegion(TreeT & tree) : m_tree(tree), m_exist(false) {}

    bool operator()(m2::PointD const & p)
    {
      // This logic is to skip last polygon point (that is equal to first).

      if (m_exist)
      {
        // add previous point to region
        m_rgn.AddPoint(D2I(m_pt));
      }
      else
        m_exist = true;

      // save current point
      m_pt = p;
      return true;
    }

    void EndRegion()
    {
      m_tree.Add(m_rgn, GetLimitRect(m_rgn));

      m_rgn = RegionT();
      m_exist = false;
    }
  };
}

void CoastlineFeaturesGenerator::AddRegionToTree(FeatureBuilder1 const & fb)
{
  ASSERT ( fb.IsGeometryClosed(), () );

  DoCreateRegion<TreeT> createRgn(m_tree);
  fb.ForEachGeometryPointEx(createRgn);
}

void CoastlineFeaturesGenerator::operator()(FeatureBuilder1 const & fb)
{
  if (fb.IsGeometryClosed())
    AddRegionToTree(fb);
  else
    m_merger(fb);
}

namespace
{
  class DoAddToTree : public FeatureEmitterIFace
  {
    CoastlineFeaturesGenerator & m_rMain;
    size_t m_notMergedCoastsCount;
    size_t m_totalNotMergedCoastsPoints;

  public:
    DoAddToTree(CoastlineFeaturesGenerator & rMain)
      : m_rMain(rMain), m_notMergedCoastsCount(0), m_totalNotMergedCoastsPoints(0) {}

    virtual void operator() (FeatureBuilder1 const & fb)
    {
      if (fb.IsGeometryClosed())
        m_rMain.AddRegionToTree(fb);
      else
      {
        LOG(LINFO, ("Not merged coastline", fb.GetOsmIdsString()));
        ++m_notMergedCoastsCount;
        m_totalNotMergedCoastsPoints += fb.GetPointsCount();
      }
    }

    bool HasNotMergedCoasts() const
    {
      return m_notMergedCoastsCount != 0;
    }

    size_t GetNotMergedCoastsCount() const
    {
      return m_notMergedCoastsCount;
    }

    size_t GetNotMergedCoastsPoints() const
    {
      return m_totalNotMergedCoastsPoints;
    }
  };
}

bool CoastlineFeaturesGenerator::Finish()
{
  DoAddToTree doAdd(*this);
  m_merger.DoMerge(doAdd);

  if (doAdd.HasNotMergedCoasts())
  {
    LOG(LINFO, ("Total not merged coasts:", doAdd.GetNotMergedCoastsCount()));
    LOG(LINFO, ("Total points in not merged coasts:", doAdd.GetNotMergedCoastsPoints()));
    if (FLAGS_fail_on_coasts)
      return false;
  }

  return true;
}

namespace
{
  class DoDifference
  {
    RectT m_src;
    vector<RegionT> m_res;
    vector<m2::PointD> m_points;

  public:
    DoDifference(RegionT const & rgn)
    {
      m_res.push_back(rgn);
      m_src = rgn.GetRect();
    }

    void operator() (RegionT const & r)
    {
      if (m_src.IsRectInside(r.GetRect()))
      {
        // if r is fully inside source rect region,
        // put it to the result vector without any intersection
        m_res.push_back(r);
      }
      else
      {
        m2::IntersectRegions(m_res.front(), r, m_res);
      }
    }

    void operator() (PointT const & p)
    {
      m_points.push_back(PointU2PointD(m2::PointU(
                                static_cast<uint32_t>(p.x),
                                static_cast<uint32_t>(p.y)), POINT_COORD_BITS));
    }

    size_t GetPointsCount() const
    {
      size_t count = 0;
      for (size_t i = 0; i < m_res.size(); ++i)
        count += m_res[i].GetPointsCount();
      return count;
    }

    void AssignGeometry(FeatureBuilder1 & fb)
    {
      for (size_t i = 0; i < m_res.size(); ++i)
      {
        m_points.clear();
        m_points.reserve(m_res[i].Size() + 1);

        m_res[i].ForEachPoint(ref(*this));

        fb.AddPolygon(m_points);
      }
    }
  };
}

bool CoastlineFeaturesGenerator::GetFeature(CellIdT const & cell, FeatureBuilder1 & fb)
{
  // get rect cell
  double minX, minY, maxX, maxY;
  CellIdConverter<MercatorBounds, CellIdT>::GetCellBounds(cell, minX, minY, maxX, maxY);

  // create rect region
  typedef m2::PointD P;
  PointT arr[] = { D2I(P(minX, minY)), D2I(P(minX, maxY)),
                   D2I(P(maxX, maxY)), D2I(P(maxX, minY)) };
  RegionT rectR(arr, arr + ARRAY_SIZE(arr));

  // Do 'and' with all regions and accumulate the result, including bound region.
  // In 'odd' parts we will have an ocean.
  DoDifference doDiff(rectR);
  m_tree.ForEachInRect(GetLimitRect(rectR), bind<void>(ref(doDiff), _1));

  // Check if too many points for feature.
  if (cell.Level() < m_highLevel && doDiff.GetPointsCount() >= m_maxPoints)
    return false;

  // assign feature
  fb.SetCoastCell(cell.ToInt64(m_highLevel + 1), cell.ToString());

  doDiff.AssignGeometry(fb);
  fb.SetArea();
  fb.AddType(m_coastType);

  // should present any geometry
  CHECK_GREATER ( fb.GetPolygonsCount(), 0, () );
  CHECK_GREATER_OR_EQUAL ( fb.GetPointsCount(),  3, () );

  return true;
}

void CoastlineFeaturesGenerator::GetFeatures(size_t i, vector<FeatureBuilder1> & vecFb)
{
  vector<CellIdT> stCells;
  stCells.push_back(CellIdT::FromBitsAndLevel(i, m_lowLevel));

  while (!stCells.empty())
  {
    CellIdT const cell = stCells.back();
    stCells.pop_back();

    vecFb.push_back(FeatureBuilder1());
    if (!GetFeature(cell, vecFb.back()))
    {
      vecFb.pop_back();

      for (int8_t i = 0; i < 4; ++i)
        stCells.push_back(cell.Child(i));
    }
  }
}
