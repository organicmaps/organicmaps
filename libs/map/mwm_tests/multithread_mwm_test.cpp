#include "testing/testing.hpp"

#include "map/features_fetcher.hpp"

#include "indexer/scales.hpp"

#include "base/macros.hpp"
#include "base/thread_pool_computational.hpp"

#include <algorithm>

namespace multithread_mwm_test
{
using SourceT = FeaturesFetcher;

class FeaturesLoader
{
public:
  explicit FeaturesLoader(SourceT const & src) : m_src(src) {}

  void operator()()
  {
    size_t const kCount = 2000;

    for (size_t i = 0; i < kCount; ++i)
    {
      m2::RectD const r = GetRandomRect();
      m_scale = scales::GetScaleLevel(r);

      m_src.ForEachFeature(r, [this](FeatureType & ft)
      {
        ft.ParseAllBeforeGeometry();

        (void)ft.GetOuterGeometryStats();
        (void)ft.GetOuterTrianglesStats();

        // Force load feature. We check asserts here. There is no any other constrains here.
        CHECK(!ft.IsEmptyGeometry(m_scale), (ft.GetID()));
      }, m_scale);
    }
  }

private:
  // Get random rect inside m_src.
  m2::RectD GetRandomRect() const
  {
    int const count = std::max(1, rand() % 50);

    int const x = rand() % count;
    int const y = rand() % count;

    m2::RectD const r = m_src.GetWorldRect();
    double const sizeX = r.SizeX() / count;
    double const sizeY = r.SizeY() / count;

    double const minX = r.minX() + x * sizeX;
    double const minY = r.minY() + y * sizeY;

    return m2::RectD(minX, minY, minX + sizeX, minY + sizeY);
  }

  SourceT const & m_src;
  int m_scale = 0;
};

void RunTest(std::string const & file)
{
  SourceT src;
  src.InitClassificator();

  UNUSED_VALUE(src.RegisterMap(platform::LocalCountryFile::MakeForTesting(file)));

  // Check that country rect is valid and not infinity.
  m2::RectD const r = src.GetWorldRect();
  TEST(r.IsValid(), ());

  m2::RectD world(mercator::Bounds::FullRect());
  world.Inflate(-10.0, -10.0);

  TEST(world.IsRectInside(r), ());

  srand(666);

  size_t const kCount = 20;
  base::ComputationalThreadPool pool(kCount);

  for (size_t i = 0; i < kCount; ++i)
    pool.SubmitWork(FeaturesLoader(src));

  pool.WaitingStop();
}

UNIT_TEST(Threading_ForEachFeature)
{
  RunTest("minsk-pass");
}

}  // namespace multithread_mwm_test
