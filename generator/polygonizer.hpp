#pragma once

#include "borders_loader.hpp"
#include "feature_builder.hpp"
#include "multiproducer_oneconsumer.hpp"

#include "../geometry/rect2d.hpp"

#include "../base/buffer_vector.hpp"

#include "../std/string.hpp"


namespace feature
{
  // Groups features according to country polygons
  template <class FeatureOutT> class Polygonizer
  {
    string m_prefix;
    string m_suffix;

    vector<FeatureOutT*> m_Buckets;
    vector<string> m_Names;
    borders::CountriesContainerT m_countries;

    MultiProducerOneConsumer m_impl;

  public:
    template <class TInfo>
    explicit Polygonizer(TInfo const & info)
      : m_prefix(info.m_datFilePrefix), m_suffix(info.m_datFileSuffix), m_impl(8)
    {
      if (info.m_splitByPolygons)
      {
        CHECK(borders::LoadCountriesList(info.m_datFilePrefix, m_countries),
            ("Error loading country polygons files"));
      }
      else
      {
        // Insert fake country polygon equal to whole world to
        // create only one output file which contains all features
        m_countries.Add(borders::CountryPolygons(),
                        m2::RectD(MercatorBounds::minX, MercatorBounds::minY,
                                  MercatorBounds::maxX, MercatorBounds::maxY));
      }
    }
    ~Polygonizer()
    {
      m_impl.Finish();
      for_each(m_Buckets.begin(), m_Buckets.end(), DeleteFunctor());
    }

    struct PointChecker
    {
      borders::RegionsContainerT const & m_regions;
      bool m_belongs;

      PointChecker(borders::RegionsContainerT const & regions)
        : m_regions(regions), m_belongs(false) {}

      bool operator()(m2::PointD const & pt)
      {
        m_regions.ForEachInRect(m2::RectD(pt, pt), bind<void>(ref(*this), _1, cref(pt)));
        return !m_belongs;
      }

      void operator() (borders::Region const & rgn, borders::Region::ValueT const & point)
      {
        if (!m_belongs)
          m_belongs = rgn.Contains(point);
      }
    };

    typedef borders::CountryPolygons PolygonsT;
    typedef buffer_vector<PolygonsT const *, 32> PolygonsVectorT;

    class InsertCountriesPtr
    {
      PolygonsVectorT & m_vec;

    public:
      InsertCountriesPtr(PolygonsVectorT & vec) : m_vec(vec) {}
      void operator() (PolygonsT const & c)
      {
        m_vec.push_back(&c);
      }
    };

    void operator () (FeatureBuilder1 const & fb)
    {
      PolygonsVectorT vec;
      m_countries.ForEachInRect(fb.GetLimitRect(), InsertCountriesPtr(vec));

      switch (vec.size())
      {
      case 0:
        break;
      case 1:
        EmitFeature(vec[0], fb);
        break;
      default:
        m_impl.RunTask(new PolygonizerTask(*this, vec, fb));
        break;
      }
    }

    void EmitFeature(PolygonsT const * country, FeatureBuilder1 const & fb)
    {
      if (country->m_index == -1)
      {
        m_Names.push_back(country->m_name);
        m_Buckets.push_back(new FeatureOutT(m_prefix + country->m_name + m_suffix));
        country->m_index = m_Buckets.size()-1;
      }

      (*(m_Buckets[country->m_index]))(fb);
    }

    inline vector<string> const & Names() const { return m_Names; }

  private:

    class PolygonizerTask : public MultiProducerOneConsumer::ITask
    {
    public:
      PolygonizerTask(Polygonizer & polygonizer,
                      PolygonsVectorT const & countries,
                      FeatureBuilder1 const & fb)
        : m_polygonizer(polygonizer), m_Countries(countries), m_FB(fb) {}

      // Override
      virtual void RunBase()
      {
        for (size_t i = 0; i < m_Countries.size(); ++i)
        {
          PointChecker doCheck(m_Countries[i]->m_regions);
          m_FB.ForEachGeometryPoint(doCheck);

          if (doCheck.m_belongs)
            Emit(const_cast<PolygonsT *>(m_Countries[i]));
        }
      }

      // Override
      virtual void EmitBase(void * p)
      {
        m_polygonizer.EmitFeature(reinterpret_cast<PolygonsT const *>(p), m_FB);
      }

    private:
      Polygonizer & m_polygonizer;

      /// @name Do copy of all input parameters.
      //@{
      PolygonsVectorT m_Countries;
      FeatureBuilder1 m_FB;
      //@}
    };
  };
}
