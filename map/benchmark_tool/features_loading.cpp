#include "../../base/SRC_FIRST.hpp"

#include "api.hpp"

#include "../feature_vec_model.hpp"

#include "../../indexer/data_factory.hpp"
#include "../../indexer/feature_visibility.hpp"

#include "../../platform/platform.hpp"

#include "../../base/timer.hpp"
#include "../../base/logging.hpp"

#include "../../std/iostream.hpp"

#include "../../base/start_mem_debug.hpp"


namespace
{
  class Accumulator
  {
    my::Timer m_timer;
    double m_reading;
    size_t m_count;

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

    void operator() (FeatureType const & ft)
    {
      ++m_count;

      m_timer.Reset();

      vector<drule::Key> keys;
      string names;       // for debug use only, in release it's empty
      (void)feature::GetDrawRule(ft, m_scale, keys, names);

      if (!keys.empty())
      {
        // Call this function to load feature's inner data and geometry.
        (void)ft.IsEmptyGeometry(m_scale);
      }

      m_reading += m_timer.ElapsedSeconds();
    }
  };

  double RunBenchmark(model::FeaturesFetcher const & src, m2::RectD const & rect,
                      pair<int, int> const & scaleR)
  {
    ASSERT_LESS_OR_EQUAL ( scaleR.first, scaleR.second, () );

    vector<m2::RectD> rects;
    rects.push_back(rect);

    Accumulator acc;

    while (!rects.empty())
    {
      m2::RectD const r = rects.back();
      rects.pop_back();

      bool doDivide = true;
      int const scale = scales::GetScaleLevel(r);
      if (scale >= scaleR.first)
      {
        acc.Reset(scale);
        src.ForEachFeature(r, acc, scale);
        doDivide = !acc.IsEmpty();
      }

      if (doDivide && scale < scaleR.second)
      {
        m2::RectD r1, r2;
        r.DivideByGreaterSize(r1, r2);
        rects.push_back(r1);
        rects.push_back(r2);
      }
    }

    return acc.GetReadingTime();
  }
}

void RunFeaturesLoadingBenchmark(string const & file, size_t count, pair<int, int> scaleR)
{
  pair<int, int> const r = GetMapScaleRange(FilesContainerR(GetPlatform().GetReader(file)));
  if (r.first > scaleR.first)
    scaleR.first = r.first;
  if (r.second < scaleR.second)
    scaleR.second = r.second;

  if (scaleR.first > scaleR.second)
    return;

  model::FeaturesFetcher src;
  src.AddMap(file);

  m2::RectD const rect = GetMapBounds(FilesContainerR(GetPlatform().GetReader(file)));

  my::Timer timer;
  double all = 0.0;
  double reading = 0.0;

  for (size_t i = 0; i < count; ++i)
  {
    timer.Reset();

    reading += RunBenchmark(src, rect, scaleR);

    all += timer.ElapsedSeconds();
  }

  // 'all time', 'index time', 'feature loading time'
  cout << all / count << ' ' << (all - reading) / count << ' ' << reading / count << endl;
}
