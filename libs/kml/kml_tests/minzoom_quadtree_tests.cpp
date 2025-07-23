#include "testing/testing.hpp"

#include "kml/minzoom_quadtree.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include <functional>
#include <limits>
#include <map>
#include <random>
#include <utility>

namespace
{
double constexpr kEps = std::numeric_limits<double>::epsilon();

m2::PointD MakeGlobalPoint(double x, double y)
{
  m2::PointD point;
  point.x = x;
  point.y = y;
  point.x *= mercator::Bounds::kRangeX;
  point.y *= mercator::Bounds::kRangeY;
  point.x += mercator::Bounds::kMinX;
  point.y += mercator::Bounds::kMinY;
  return point;
}

}  // namespace

UNIT_TEST(Kml_MinzoomQuadtree_PopulationGrowthRate)
{
  {
    kml::MinZoomQuadtree<double /* rank */, std::less<double>> minZoomQuadtree{{} /* less */};
    size_t const kMaxDepth = 5;
    double const step = 1.0 / (1 << kMaxDepth);
    for (int x = 0; x < (1 << kMaxDepth); ++x)
      for (int y = 0; y < (1 << kMaxDepth); ++y)
        minZoomQuadtree.Add(MakeGlobalPoint((0.5 + x) * step, (0.5 + y) * step), 0.0 /* rank */);
    double const kCountPerTile = 1.0;
    std::map<int /* zoom */, size_t> populationCount;
    auto const incZoomPopulation = [&](double & rank, int zoom)
    {
      TEST_ALMOST_EQUAL_ABS(rank, 0.0, kEps, ());
      ++populationCount[zoom];
    };
    minZoomQuadtree.SetMinZoom(kCountPerTile, scales::GetUpperStyleScale(), incZoomPopulation);
    TEST_EQUAL(populationCount.size(), kMaxDepth + 1, (populationCount));

    auto p = populationCount.cbegin();
    ASSERT(p != populationCount.cend(), ());
    std::vector<size_t> partialSums;
    partialSums.push_back(p->second);
    while (++p != populationCount.cend())
      partialSums.push_back(partialSums.back() + p->second);
    TEST_EQUAL(partialSums.front(), 1, ());
    TEST_EQUAL(partialSums.back(), (1 << (kMaxDepth * 2)), ());
    auto const isGrowthExponential = [](size_t lhs, size_t rhs) { return rhs != 4 * lhs; };
    TEST(std::adjacent_find(partialSums.cbegin(), partialSums.cend(), isGrowthExponential) == partialSums.cend(),
         (partialSums));
  }

  std::mt19937 g(0);

  auto const gen = [&g] { return std::generate_canonical<double, std::numeric_limits<uint32_t>::digits>(g); };

  for (int i = 0; i < 5; ++i)
  {
    kml::MinZoomQuadtree<double /* rank */, std::less<double>> minZoomQuadtree{{} /* less */};

    size_t const kTotalCount = 1 + g() % 10000;
    for (size_t i = 0; i < kTotalCount; ++i)
    {
      auto const x = gen();
      auto const y = gen();
      auto const rank = gen();
      minZoomQuadtree.Add(MakeGlobalPoint(x, y), rank);
    }

    double const kCountPerTile = 1.0 + kTotalCount * gen() / 2.0;
    std::map<int /* zoom */, size_t> populationCount;
    auto const incZoomPopulation = [&](double & /* rank */, int zoom) { ++populationCount[zoom]; };
    minZoomQuadtree.SetMinZoom(kCountPerTile, scales::GetUpperStyleScale(), incZoomPopulation);

    auto p = populationCount.cbegin();
    ASSERT(p != populationCount.cend(), ());
    std::vector<size_t> partialSums;
    partialSums.push_back(p->second);
    while (++p != populationCount.cend())
      partialSums.push_back(partialSums.back() + p->second);
    TEST_EQUAL(partialSums.back(), kTotalCount, ());

    double areaScale = 1.0;
    for (size_t i = 0; i < partialSums.size(); ++i)
    {
      auto const maxAbsErr = 4.0 * std::ceil(std::sqrt(kCountPerTile * areaScale)) + 4.0;
      TEST_LESS_OR_EQUAL(partialSums[i], kCountPerTile * areaScale + maxAbsErr,
                         (kCountPerTile, maxAbsErr, partialSums));
      areaScale *= 4.0;
    }
  }
}

UNIT_TEST(Kml_MinzoomQuadtree_CornerCases)
{
  {
    kml::MinZoomQuadtree<double /* rank */, std::less<double>> minZoomQuadtree{{} /* less */};
    minZoomQuadtree.Add(MakeGlobalPoint(0.5, 0.5), 0.0 /* rank */);
    double const kCountPerTile = 100.0;
    auto const checkRank = [&](double & rank, int zoom)
    {
      TEST_ALMOST_EQUAL_ABS(rank, 0.0, kEps, ());
      TEST_EQUAL(zoom, 1, ());
    };
    minZoomQuadtree.SetMinZoom(kCountPerTile, scales::GetUpperStyleScale(), checkRank);
  }

  {
    kml::MinZoomQuadtree<double /* rank */, std::less<double>> minZoomQuadtree{{} /* less */};
    double const kCountPerTile = 100.0;
    auto const unreachable = [&](double & /* rank */, int /* zoom */) { TEST(false, ()); };
    minZoomQuadtree.SetMinZoom(kCountPerTile, scales::GetUpperStyleScale(), unreachable);
  }
}

UNIT_TEST(Kml_MinzoomQuadtree_MaxZoom)
{
  kml::MinZoomQuadtree<double /* rank */, std::less<double>> minZoomQuadtree{{} /* less */};
  size_t const kMaxDepth = 5;
  double const step = 1.0 / (1 << kMaxDepth);
  for (int x = 0; x < (1 << kMaxDepth); ++x)
    for (int y = 0; y < (1 << kMaxDepth); ++y)
      minZoomQuadtree.Add(MakeGlobalPoint((0.5 + x) * step, (0.5 + y) * step), 0.0 /* rank */);
  double const kCountPerTile = 1.0;
  std::map<int /* zoom */, size_t> populationCount;
  auto const incZoomPopulation = [&](double & rank, int zoom)
  {
    TEST_ALMOST_EQUAL_ABS(rank, 0.0, kEps, ());
    ++populationCount[zoom];
  };
  int const kMaxZoom = 4;
  minZoomQuadtree.SetMinZoom(kCountPerTile, kMaxZoom, incZoomPopulation);
  TEST_EQUAL(populationCount.size(), kMaxZoom, (populationCount));

  auto p = populationCount.cbegin();
  ASSERT(p != populationCount.cend(), ());
  std::vector<size_t> partialSums;
  partialSums.push_back(p->second);
  while (++p != populationCount.cend())
    partialSums.push_back(partialSums.back() + p->second);
  TEST_EQUAL(partialSums.front(), 1, ());
  TEST_EQUAL(partialSums.back(), (1 << (kMaxDepth * 2)), ());
  auto const isGrowthExponential = [](size_t lhs, size_t rhs) { return rhs != 4 * lhs; };
  TEST(std::adjacent_find(partialSums.cbegin(), std::prev(partialSums.cend()), isGrowthExponential) ==
           std::prev(partialSums.cend()),
       (partialSums));
  TEST_LESS_OR_EQUAL(4 * *std::prev(partialSums.cend(), 2), partialSums.back(), ());
}
