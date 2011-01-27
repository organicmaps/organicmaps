#pragma once

#include "../../base/base.hpp"

#include "../../coding/file_writer.hpp"

#include "../../geometry/rect2d.hpp"

#include "../../indexer/feature.hpp"
#include "../../indexer/feature_visibility.hpp"
#include "../../indexer/cell_id.hpp"

#include "../../std/string.hpp"

#include <boost/scoped_ptr.hpp>

#include "kml_parser.hpp"

namespace feature
{
  // Groups features according to country polygons
  template <class FeatureOutT, class BoundsT, typename CellIdT>
  class Polygonizer
  {
    typedef CellIdConverter<BoundsT, RectId> CellIdConverterType;

  public:
    template <class TInfo>
    Polygonizer(TInfo & info)
    : m_FeatureOutInitData(info.datFilePrefix, info.datFileSuffix), m_Names(info.bucketNames)
    {
      CHECK(kml::LoadCountriesList(info.datFilePrefix, m_countries, info.m_simplifyCountriesLevel),
            ("Error loading polygons"));

      //LOG_SHORT(LINFO, ("Loaded polygons count for regions:"));
      //for (size_t i = 0; i < m_countries.size(); ++i)
      //{
      //  LOG_SHORT(LINFO, (m_countries[i].m_name, m_countries[i].m_regions.size()));
      //}
    }
    ~Polygonizer()
    {
      for_each(m_Buckets.begin(), m_Buckets.end(), DeleteFunctor());
    }

    static m2::PointU Mercator2CellId(m2::PointD const & pt)
    {
      return m2::PointU(static_cast<uint32_t>(CellIdConverterType::XToCellIdX(pt.x)),
                        static_cast<uint32_t>(CellIdConverterType::YToCellIdY(pt.y)));
    }

    struct PointChecker
    {
      kml::RegionsContainerT const & m_regions;
      bool m_belongs;

      PointChecker(kml::RegionsContainerT const & regions)
        : m_regions(regions), m_belongs(false) {}

      bool operator()(m2::PointD const & pt)
      {
        kml::Region::value_type const point = Mercator2CellId(pt);

        m_regions.ForEachInRect(m2::RectD(point, point), bind<void>(ref(*this), _1, cref(point)));

        return !m_belongs;
      }

      void operator() (kml::Region const & rgn, kml::Region::value_type const & point)
      {
        if (!m_belongs)
          m_belongs = rgn.Contains(point);
      }
    };

    void operator () (FeatureBuilder1 const & fb)
    {
      m2::RectD const r = fb.GetLimitRect();
      m_countries.ForEachInRect(
          m2::RectD(Mercator2CellId(r.LeftBottom()), Mercator2CellId(r.RightTop())),
          bind<void>(ref(*this), _1, cref(fb)));
    }

    void operator() (kml::CountryPolygons const & country, FeatureBuilder1 const & fb)
    {
      PointChecker doCheck(country.m_regions);
      fb.ForEachTruePointRef(doCheck);

      if (doCheck.m_belongs)
      {
        if (country.m_index == -1)
        {
          m_Names.push_back(country.m_name);
          m_Buckets.push_back(new FeatureOutT(country.m_name, m_FeatureOutInitData));
          country.m_index = m_Buckets.size()-1;
        }

        (*(m_Buckets[country.m_index]))(fb);
      }
    }

  private:
    typename FeatureOutT::InitDataType m_FeatureOutInitData;

    vector<FeatureOutT*> m_Buckets;
    vector<string> m_Names;

    kml::CountriesContainerT m_countries;
  };
}
