#include "../../base/SRC_FIRST.hpp"

#include "api.hpp"

#include "../feature_vec_model.hpp"

#include "../../indexer/data_factory.hpp"
#include "../../indexer/feature_visibility.hpp"

#include "../../platform/platform.hpp"

#include "../../base/timer.hpp"

#include "../../base/start_mem_debug.hpp"


namespace bench
{

namespace
{
  class Accumulator
  {
    my::Timer m_timer;
    size_t m_count;

    int m_scale;

  public:
    Result m_res;

    void Reset(int scale)
    {
      m_scale = scale;
      m_count = 0;
    }

    bool IsEmpty() const { return m_count == 0; }

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

      m_res.Add(m_timer.ElapsedSeconds());
    }
  };

  Result RunBenchmark(model::FeaturesFetcher const & src, m2::RectD const & rect,
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

    return acc.m_res;
  }
}

AllResult RunFeaturesLoadingBenchmark(string const & file, size_t count, pair<int, int> scaleR)
{
  feature::DataHeader header;
  LoadMapHeader(GetPlatform().GetReader(file), header);

  pair<int, int> const r = header.GetScaleRange();
  if (r.first > scaleR.first)
    scaleR.first = r.first;
  if (r.second < scaleR.second)
    scaleR.second = r.second;

  if (scaleR.first > scaleR.second)
    return AllResult();

  model::FeaturesFetcher src;
  src.AddMap(file);

  m2::RectD const rect = header.GetBounds();

  my::Timer timer;
  AllResult res;

  for (size_t i = 0; i < count; ++i)
  {
    timer.Reset();

    res.m_reading.Add(RunBenchmark(src, rect, scaleR));

    res.m_all.Add(timer.ElapsedSeconds());
  }

  return res;
}

}
