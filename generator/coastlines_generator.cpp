#include "coastlines_generator.hpp"
#include "feature_builder.hpp"

#include "../indexer/point_to_int64.hpp"

#include "../geometry/region2d/binary_operators.hpp"

#include "../base/string_utils.hpp"

#include "../std/bind.hpp"


typedef m2::RegionI RegionT;


CoastlineFeaturesGenerator::CoastlineFeaturesGenerator(uint32_t coastType, int level)
  : m_merger(POINT_COORD_BITS), m_coastType(coastType), m_Level(level)
{
}

namespace
{
  m2::RectD GetLimitRect(RegionT const & rgn)
  {
    m2::RectI r = rgn.GetRect();
    return m2::RectD(r.minX(), r.minY(), r.maxX(), r.maxY());
  }

  inline m2::PointI D2I(m2::PointD const & p)
  {
    m2::PointU const pu = PointD2PointU(p, POINT_COORD_BITS);
    return m2::PointI(static_cast<int32_t>(pu.x),
                      static_cast<int32_t>(pu.y));
  }

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
}

void CoastlineFeaturesGenerator::AddRegionToTree(FeatureBuilder1 const & fb)
{
  ASSERT ( fb.IsGeometryClosed(), () );

  DoCreateRegion createRgn;
  fb.ForEachGeometryPoint(createRgn);
  createRgn.Add(m_tree);
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
    }
  };
}

void CoastlineFeaturesGenerator::Finish()
{
  DoAddToTree doAdd(*this);
  m_merger.DoMerge(doAdd);
}

namespace
{
  class DoDifference
  {
    vector<RegionT> m_res;
    vector<m2::PointD> m_points;

  public:
    DoDifference(RegionT const & rgn)
    {
      m_res.push_back(rgn);
    }

    void operator() (RegionT const & r)
    {
      vector<RegionT> local;

      for (size_t i = 0; i < m_res.size(); ++i)
        m2::DiffRegions(m_res[i], r, local);

      local.swap(m_res);
    }

    void operator() (m2::PointI const & p)
    {
      CoordPointT const c = PointU2PointD(m2::PointU(
                                static_cast<uint32_t>(p.x),
                                static_cast<uint32_t>(p.y)), POINT_COORD_BITS);
      m_points.push_back(m2::PointD(c.first, c.second));
    }

    void AssignGeometry(FeatureBuilder1 & fb)
    {
      for (size_t i = 0; i < m_res.size(); ++i)
      {
        m_points.clear();
        m_points.reserve(m_res[i].Size() + 1);

        m_res[i].ForEachPoint(bind<void>(ref(*this), _1));

        fb.AddPolygon(m_points);
      }
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
  m2::PointI arr[] = { D2I(P(minX, minY)), D2I(P(minX, maxY)),
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

  return true;
}
