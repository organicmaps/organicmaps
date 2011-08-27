#include "../../base/SRC_FIRST.hpp"

#include "api.hpp"

#include "../feature_vec_model.hpp"

#include "../../indexer/data_factory.hpp"

#include "../../platform/platform.hpp"

#include "../../base/timer.hpp"
#include "../../base/logging.hpp"

#include "../../std/iostream.hpp"

#include "../../base/start_mem_debug.hpp"


namespace
{
  class Accumulator
  {
    mutable my::Timer m_timer;
    mutable double m_reading;
    mutable size_t m_count;

    int m_scale;

  public:
    Accumulator() : m_reading(0.0) {}

    void Reset(int scale)
    {
      m_scale = scale;
      m_count = 0;
    }

    bool IsEmpty() const { return m_count == 0; }

    double GetReadingTime() const { return m_reading; }

    void operator() (FeatureType const & ft) const
    {
      ++m_count;

      m_timer.Reset();

      // Call this function to load feature's inner data and geometry.
      (void)ft.IsEmptyGeometry(m_scale);

      m_reading += m_timer.ElapsedSeconds();
    }
  };

  double RunBenchmark(model::FeaturesFetcher const & src, m2::RectD const & rect,
                      pair<int, int> const & scaleRange)
  {
    vector<m2::RectD> rects, newRects;
    rects.push_back(rect);


    Accumulator acc;

    for (int scale = scaleRange.first; scale < scaleRange.second; ++scale)
    {
      for (size_t i = 0; i < rects.size(); ++i)
      {
        m2::RectD const r = rects[i];

        acc.Reset(scale);

        src.ForEachFeature_TileDrawing(r, acc, scale);

        m2::RectD r1, r2;
        r.DivideByGreaterSize(r1, r2);
        newRects.push_back(r1);
        newRects.push_back(r2);
      }
      rects.swap(newRects);
      newRects.clear();
    }
    return acc.GetReadingTime();
  }
}

void RunFeaturesLoadingBenchmark(string const & file, size_t count)
{
  model::FeaturesFetcher src;
  src.InitClassificator();
  src.AddMap(file);

  m2::RectD const rect = GetMapBounds(FilesContainerR(GetPlatform().GetReader(file)));
  pair<int, int> const scaleRange = GetMapScaleRange(FilesContainerR(GetPlatform().GetReader(file)));

  my::Timer timer;
  double all = 0.0;
  double reading = 0.0;

  for (size_t i = 0; i < count; ++i)
  {
    timer.Reset();

    reading += RunBenchmark(src, rect, scaleRange);

    all += timer.ElapsedSeconds();
  }

  // 'all time', 'index time', 'feature loading time'
  cout << all / count << ' ' << (all - reading) / count << ' ' << reading / count << endl;
}
