#include "coastlines_generator.hpp"
#include "feature_builder.hpp"
#include "tesselator.hpp"

#include "../indexer/point_to_int64.hpp"

#include "../geometry/region2d/binary_operators.hpp"

#include "../base/string_utils.hpp"
#include "../base/logging.hpp"

#include "../std/bind.hpp"


typedef m2::RegionI RegionT;
typedef m2::PointI PointT;
typedef m2::RectI RectT;


CoastlineFeaturesGenerator::CoastlineFeaturesGenerator(uint32_t coastType, int level)
  : m_merger(POINT_COORD_BITS), m_coastType(coastType), m_Level(level)
{
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

  /*
  class DoCreateRegion
  {
    RegionT m_rgn;
    m2::PointD m_pt;
    bool m_exist;

  public:
    DoCreateRegion() : m_exist(false) {}

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

    template <class TreeT> void Add(TreeT & tree)
    {
      tree.Add(m_rgn, GetLimitRect(m_rgn));
    }
  };
  */

  template <class ContainerT> class DoAccumulate
  {
    ContainerT & m_list;
  public:
    DoAccumulate(ContainerT & lst) : m_list(lst) {}
    bool operator() (m2::PointD const & p)
    {
      m_list.back().push_back(p);
      return true;
    }
  };
}

void CoastlineFeaturesGenerator::AddRegionToTree(FeatureBuilder1 const & fb)
{
  ASSERT ( fb.IsGeometryClosed(), () );

  //DoCreateRegion createRgn;
  DoAccumulate<RegionsT> createRgn(m_regions);
  fb.ForEachGeometryPoint(createRgn);
  //createRgn.Add(m_tree);
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
  public:
    DoAddToTree(CoastlineFeaturesGenerator & rMain) : m_rMain(rMain) {}

    virtual void operator() (FeatureBuilder1 const & fb)
    {
      if (fb.IsGeometryClosed())
        m_rMain.AddRegionToTree(fb);
      else
        LOG(LINFO, ("Not merged coastline", fb));
    }
  };

  template <class TreeT> class DoMakeRegions
  {
    TreeT & m_tree;
  public:
    DoMakeRegions(TreeT & tree) : m_tree(tree) {}

    void operator() (m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
    {
      RegionT rgn;
      rgn.AddPoint(D2I(p1));
      rgn.AddPoint(D2I(p2));
      rgn.AddPoint(D2I(p3));
      m_tree.Add(rgn, GetLimitRect(rgn));
    }
  };
}

void CoastlineFeaturesGenerator::Finish()
{
  DoAddToTree doAdd(*this);
  m_merger.DoMerge(doAdd);

  LOG(LINFO, ("Start continents tesselation"));

  tesselator::TrianglesInfo info;
  tesselator::TesselateInterior(m_regions, info);
  info.ForEachTriangle(DoMakeRegions<TreeT>(m_tree));

  LOG(LINFO, ("End continents tesselation"));
}

namespace
{
  class DoDifference
  {
    RectT m_src;
    vector<RegionT> m_res, m_readyRes;
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
        m_readyRes.push_back(r);
      }
      else
      {
        vector<RegionT> local;

        for (size_t i = 0; i < m_res.size(); ++i)
          m2::DiffRegions(m_res[i], r, local);

        local.swap(m_res);
      }
    }

    void operator() (PointT const & p)
    {
      CoordPointT const c = PointU2PointD(m2::PointU(
                                static_cast<uint32_t>(p.x),
                                static_cast<uint32_t>(p.y)), POINT_COORD_BITS);
      m_points.push_back(m2::PointD(c.first, c.second));
    }

    void AssignGeometry(vector<RegionT> const & v, FeatureBuilder1 & fb)
    {
      for (size_t i = 0; i < v.size(); ++i)
      {
        m_points.clear();
        m_points.reserve(v[i].Size() + 1);

        v[i].ForEachPoint(bind<void>(ref(*this), _1));

        fb.AddPolygon(m_points);
      }
    }

    void AssignGeometry(FeatureBuilder1 & fb)
    {
      AssignGeometry(m_res, fb);
      AssignGeometry(m_readyRes, fb);
    }
  };

  class DoLogRegions
  {
  public:
    void operator() (RegionT const & r)
    {
      LOG_SHORT(LINFO, ("Boundary", r));
    }
  };
}

bool CoastlineFeaturesGenerator::GetFeature(size_t i, FeatureBuilder1 & fb)
{
  // get rect cell
  CellIdT cell = CellIdT::FromBitsAndLevel(i, m_Level);
  double minX, minY, maxX, maxY;
  CellIdConverter<MercatorBounds, CellIdT>::GetCellBounds(cell, minX, minY, maxX, maxY);

  // create rect region
  typedef m2::PointD P;
  PointT arr[] = { D2I(P(minX, minY)), D2I(P(minX, maxY)),
                   D2I(P(maxX, maxY)), D2I(P(maxX, minY)) };
  RegionT rectR(arr, arr + ARRAY_SIZE(arr));

  // substract all 'land' from this region
  DoDifference doDiff(rectR);
  m_tree.ForEachInRect(GetLimitRect(rectR), bind<void>(ref(doDiff), _1));

  // assign feature
  FeatureParams params;
  params.name.AddString(0, strings::to_string(i));
  fb.SetParams(params);

  doDiff.AssignGeometry(fb);
  fb.SetArea();
  fb.AddType(m_coastType);
  fb.SetCoastCell(i);

  // should present any geometry
  return (fb.GetPointsCount() >= 3);
}
