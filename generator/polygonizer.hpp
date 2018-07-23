#pragma once

#include "generator/borders_loader.hpp"
#include "generator/feature_builder.hpp"
#include "generator/generate_info.hpp"
#include "generator/osm_source.hpp"

#include "indexer/feature_visibility.hpp"
#include "indexer/cell_id.hpp"

#include "geometry/rect2d.hpp"

#include "coding/file_writer.hpp"

#include "base/base.hpp"
#include "base/buffer_vector.hpp"
#include "base/macros.hpp"

#include <string>

#ifndef PARALLEL_POLYGONIZER
#define PARALLEL_POLYGONIZER 1
#endif

#if PARALLEL_POLYGONIZER
#include <QtCore/QSemaphore>
#include <QtCore/QThreadPool>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#endif

namespace feature
{
  // Groups features according to country polygons
  template <class FeatureOutT>
  class Polygonizer
  {
    feature::GenerateInfo const & m_info;

    vector<FeatureOutT*> m_Buckets;
    vector<std::string> m_Names;
    borders::CountriesContainer m_countries;

#if PARALLEL_POLYGONIZER
    QThreadPool m_ThreadPool;
    QSemaphore m_ThreadPoolSemaphore;
    QMutex m_EmitFeatureMutex;
#endif

  public:
    Polygonizer(feature::GenerateInfo const & info)
      : m_info(info)
#if PARALLEL_POLYGONIZER
      , m_ThreadPoolSemaphore(m_ThreadPool.maxThreadCount() * 8)
#endif
    {
#if PARALLEL_POLYGONIZER
      LOG(LINFO, ("Polygonizer thread pool threads:", m_ThreadPool.maxThreadCount()));
#endif

      if (info.m_splitByPolygons)
      {
        CHECK(borders::LoadCountriesList(info.m_targetDir, m_countries),
            ("Error loading country polygons files"));
      }
      else
      {
        // Insert fake country polygon equal to whole world to
        // create only one output file which contains all features
        m_countries.Add(borders::CountryPolygons(info.m_fileName), MercatorBounds::FullRect());
      }
    }
    ~Polygonizer()
    {
      Finish();
      for_each(m_Buckets.begin(), m_Buckets.end(), DeleteFunctor());
    }

    struct PointChecker
    {
      borders::RegionsContainer const & m_regions;
      bool m_belongs;

      PointChecker(borders::RegionsContainer const & regions)
        : m_regions(regions), m_belongs(false) {}

      bool operator()(m2::PointD const & pt)
      {
        m_regions.ForEachInRect(m2::RectD(pt, pt),
                                std::bind<void>(std::ref(*this), std::placeholders::_1, std::cref(pt)));
        return !m_belongs;
      }

      void operator() (borders::Region const & rgn, borders::Region::Value const & point)
      {
        if (!m_belongs)
          m_belongs = rgn.Contains(point);
      }
    };

    class InsertCountriesPtr
    {
      typedef buffer_vector<borders::CountryPolygons const *, 32> vec_type;
      vec_type & m_vec;

    public:
      InsertCountriesPtr(vec_type & vec) : m_vec(vec) {}
      void operator() (borders::CountryPolygons const & c)
      {
        m_vec.push_back(&c);
      }
    };

    void operator()(FeatureBuilder1 & fb)
    {
      buffer_vector<borders::CountryPolygons const *, 32> vec;
      m_countries.ForEachInRect(fb.GetLimitRect(), InsertCountriesPtr(vec));

      switch (vec.size())
      {
      case 0:
        break;
      case 1:
        EmitFeature(vec[0], fb);
        break;
      default:
        {
#if PARALLEL_POLYGONIZER
          m_ThreadPoolSemaphore.acquire();
          m_ThreadPool.start(new PolygonizerTask(this, vec, fb));
#else
        PolygonizerTask task(this, vec, fb);
        task.RunBase();
#endif
        }
      }
    }

    std::string m_currentNames;

    void Start()
    {
      m_currentNames.clear();
    }

    void Finish()
    {
#if PARALLEL_POLYGONIZER
      m_ThreadPool.waitForDone();
#endif
    }

    void EmitFeature(borders::CountryPolygons const * country, FeatureBuilder1 const & fb)
    {
#if PARALLEL_POLYGONIZER
      QMutexLocker mutexLocker(&m_EmitFeatureMutex);
      UNUSED_VALUE(mutexLocker);
#endif
      if (country->m_index == -1)
      {
        m_Names.push_back(country->m_name);
        m_Buckets.push_back(new FeatureOutT(m_info.GetTmpFileName(country->m_name)));
        country->m_index = static_cast<int>(m_Buckets.size())-1;
      }

      if (!m_currentNames.empty())
        m_currentNames += ';';
      m_currentNames += country->m_name;

      auto & bucket = *(m_Buckets[country->m_index]);
      bucket(fb);
    }

    vector<std::string> const & Names() const
    {
      return m_Names;
    }

  private:
    friend class PolygonizerTask;

    class PolygonizerTask
#if PARALLEL_POLYGONIZER
      : public QRunnable
#endif
    {
    public:
      PolygonizerTask(Polygonizer * pPolygonizer,
                      buffer_vector<borders::CountryPolygons const *, 32> const & countries,
                      FeatureBuilder1 const & fb)
        : m_pPolygonizer(pPolygonizer), m_Countries(countries), m_fb(fb)
      {
      }

      void RunBase()
      {
        for (size_t i = 0; i < m_Countries.size(); ++i)
        {
          PointChecker doCheck(m_Countries[i]->m_regions);
          m_fb.ForEachGeometryPoint(doCheck);

          if (doCheck.m_belongs)
            m_pPolygonizer->EmitFeature(m_Countries[i], m_fb);
        }
      }

#if PARALLEL_POLYGONIZER
      void run()
      {
        RunBase();

        m_pPolygonizer->m_ThreadPoolSemaphore.release();
      }
#endif

    private:
      Polygonizer * m_pPolygonizer;
      buffer_vector<borders::CountryPolygons const *, 32> m_Countries;
      FeatureBuilder1 m_fb;
    };
  };
}  // namespace feature
