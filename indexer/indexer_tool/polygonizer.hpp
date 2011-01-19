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
  public:
    Polygonizer(string const & dir, typename FeatureOutT::InitDataType const & featureOutInitData,
                int simplifyCountriesLevel)
    : m_FeatureOutInitData(featureOutInitData)
    {
      CHECK(kml::LoadCountriesList(dir, m_countries, simplifyCountriesLevel), ("Error loading polygons"));
      LOG_SHORT(LINFO, ("Loaded polygons count for regions:"));
      for (size_t i = 0; i < m_countries.size(); ++i)
      {
        LOG_SHORT(LINFO, (m_countries[i].m_name, m_countries[i].m_regions.size()));
      }

      m_Buckets.resize(m_countries.size());
    }

    struct PointChecker
    {
      typedef CellIdConverter<BoundsT, RectId> CellIdConverterType;

      kml::RegionsContainerT const & m_regions;
      bool m_belongs;

      PointChecker(kml::RegionsContainerT const & regions)
        : m_regions(regions), m_belongs(false) {}

      bool operator()(m2::PointD const & pt)
      {
        for (size_t i = 0; i < m_regions.size(); ++i)
        {
          kml::Region::value_type const point(static_cast<uint32_t>(CellIdConverterType::XToCellIdX(pt.x)),
               static_cast<uint32_t>(CellIdConverterType::YToCellIdY(pt.y)));
          if (m_regions[i].Contains(point))
          {
            m_belongs = true;
            // stop points processing
            return false;
          }
        }
        // continue with next point
        return true;
      }
    };

    void operator () (FeatureBuilder1 const & fb)
    {
      m2::RectD const limitRect = fb.GetLimitRect();
      for (uint32_t i = 0; i < m_Buckets.size(); ++i)
      {
        // First quick and dirty limit rect intersection.
        if (m_countries[i].m_rect.IsIntersect(limitRect))
        {
          PointChecker isPointContained(m_countries[i].m_regions);
          // feature can be without geometry but with only center point
          if (fb.GetPointsCount())
            fb.ForEachTruePointRef(isPointContained);
          else
            isPointContained.operator ()(fb.CenterPoint());

          if (isPointContained.m_belongs)
          {
            if (!m_Buckets[i].m_pOut)
              m_Buckets[i].m_pOut = new FeatureOutT(BucketName(i), m_FeatureOutInitData);

            (*(m_Buckets[i].m_pOut))(fb);
          }
        }
      }
    }

    template <typename F> void GetBucketNames(F f) const
    {
      for (uint32_t i = 0; i < m_Buckets.size(); ++i)
        if (m_Buckets[i].m_pOut)
          f(BucketName(i));
    }

  private:
    inline string BucketName(uint32_t i) const
    {
      return m_countries[i].m_name;
    }

    struct Bucket
    {
      Bucket() : m_pOut(NULL) {}
      ~Bucket() { delete m_pOut; }

      FeatureOutT * m_pOut;
    };

    typename FeatureOutT::InitDataType m_FeatureOutInitData;
    vector<Bucket> m_Buckets;
    kml::CountriesContainerT m_countries;
  };
}
