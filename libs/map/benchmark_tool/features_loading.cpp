#include "map/benchmark_tool/api.hpp"

#include "map/features_fetcher.hpp"

#include "indexer/feature_visibility.hpp"
#include "indexer/scales.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
#include "base/macros.hpp"
#include "base/timer.hpp"

#include <utility>
#include <vector>

using namespace std;

namespace bench
{
namespace
{
class Accumulator
{
public:
  explicit Accumulator(Result & res) : m_res(res) {}

  void Reset(int scale)
  {
    m_scale = scale;
    m_count = 0;
  }

  bool IsEmpty() const { return m_count == 0; }

  void operator()(FeatureType & ft)
  {
    ++m_count;

    m_timer.Reset();

    drule::KeysT keys;
    UNUSED_VALUE(feature::GetDrawRule(feature::TypesHolder(ft), m_scale, keys));

    if (!keys.empty())
    {
      // Call this function to load feature's inner data and geometry.
      UNUSED_VALUE(ft.IsEmptyGeometry(m_scale));
    }

    m_res.Add(m_timer.ElapsedSeconds());
  }

private:
  base::Timer m_timer;
  size_t m_count = 0;

  Result & m_res;

  int m_scale = 0;
};

void RunBenchmark(FeaturesFetcher const & src, m2::RectD const & rect, pair<int, int> const & scaleRange,
                  AllResult & res)
{
  ASSERT_LESS_OR_EQUAL(scaleRange.first, scaleRange.second, ());

  vector<m2::RectD> rects;
  rects.push_back(rect);

  Accumulator acc(res.m_reading);

  while (!rects.empty())
  {
    m2::RectD const r = rects.back();
    rects.pop_back();

    bool doDivide = true;
    int const scale = scales::GetScaleLevel(r);
    if (scale >= scaleRange.first)
    {
      acc.Reset(scale);

      base::Timer timer;
      src.ForEachFeature(r, acc, scale);
      res.Add(timer.ElapsedSeconds());

      doDivide = !acc.IsEmpty();
    }

    if (doDivide && scale < scaleRange.second)
    {
      m2::RectD r1, r2;
      r.DivideByGreaterSize(r1, r2);
      rects.push_back(r1);
      rects.push_back(r2);
    }
  }
}
}  // namespace

void RunFeaturesLoadingBenchmark(string fileName, pair<int, int> scaleRange, AllResult & res)
{
  base::GetNameFromFullPath(fileName);
  base::GetNameWithoutExt(fileName);

  FeaturesFetcher src;
  auto const r = src.RegisterMap(platform::LocalCountryFile::MakeForTesting(std::move(fileName)));
  if (r.second != MwmSet::RegResult::Success)
    return;

  uint8_t const minScale = r.first.GetInfo()->m_minScale;
  uint8_t const maxScale = r.first.GetInfo()->m_maxScale;
  if (minScale > scaleRange.first)
    scaleRange.first = minScale;
  if (maxScale < scaleRange.second)
    scaleRange.second = maxScale;

  if (scaleRange.first > scaleRange.second)
    return;

  RunBenchmark(src, r.first.GetInfo()->m_bordersRect, scaleRange, res);
}
}  // namespace bench
