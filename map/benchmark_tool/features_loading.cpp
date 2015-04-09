#include "map/benchmark_tool/api.hpp"

#include "map/feature_vec_model.hpp"

#include "indexer/data_factory.hpp"
#include "indexer/data_header.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/scales.hpp"

#include "platform/platform.hpp"

#include "base/macros.hpp"
#include "base/timer.hpp"

namespace bench
{

namespace
{
  class Accumulator
  {
    my::Timer m_timer;
    size_t m_count;

    Result & m_res;

    int m_scale;

  public:
    Accumulator(Result & res) : m_res(res) {}

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

      drule::KeysT keys;
      (void)feature::GetDrawRule(ft, m_scale, keys);

      if (!keys.empty())
      {
        // Call this function to load feature's inner data and geometry.
        (void)ft.IsEmptyGeometry(m_scale);
      }

      m_res.Add(m_timer.ElapsedSeconds());
    }
  };

  void RunBenchmark(model::FeaturesFetcher const & src, m2::RectD const & rect,
                    pair<int, int> const & scaleR, AllResult & res)
  {
    ASSERT_LESS_OR_EQUAL ( scaleR.first, scaleR.second, () );

    vector<m2::RectD> rects;
    rects.push_back(rect);

    Accumulator acc(res.m_reading);

    while (!rects.empty())
    {
      m2::RectD const r = rects.back();
      rects.pop_back();

      bool doDivide = true;
      int const scale = scales::GetScaleLevel(r);
      if (scale >= scaleR.first)
      {
        acc.Reset(scale);

        my::Timer timer;
        src.ForEachFeature(r, acc, scale);
        res.Add(timer.ElapsedSeconds());

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
  }
}

void RunFeaturesLoadingBenchmark(string const & file, pair<int, int> scaleR, AllResult & res)
{
  feature::DataHeader header;
  LoadMapHeader(GetPlatform().GetReader(file), header);

  pair<int, int> const r = header.GetScaleRange();
  if (r.first > scaleR.first)
    scaleR.first = r.first;
  if (r.second < scaleR.second)
    scaleR.second = r.second;

  if (scaleR.first > scaleR.second)
    return;

  model::FeaturesFetcher src;
  UNUSED_VALUE(src.RegisterMap(file));

  RunBenchmark(src, header.GetBounds(), scaleR, res);
}

}
