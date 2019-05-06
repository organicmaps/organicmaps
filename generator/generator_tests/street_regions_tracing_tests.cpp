#include "testing/testing.hpp"

#include "generator/streets/street_regions_tracing.hpp"

#include "base/geo_object_id.hpp"

using namespace generator::streets;
using generator::KeyValue;

UNIT_TEST(StreetRegionTracingTest_StreetInOneRegion)
{
  auto regionGetter = [] (auto &&) { return KeyValue{1, {}}; };
  auto && tracing = StreetRegionsTracing{{{0.0, 0.0}, {1.0, 1.0}}, regionGetter};
  auto && segments = tracing.StealPathSegments();

  TEST_EQUAL(segments.size(), 1, ());
  TEST_EQUAL(segments.front().m_region.first, 1, ());
}

UNIT_TEST(StreetRegionTracingTest_OutgoingStreet)
{
  auto regionGetter = [] (auto && point) {
    if (point.x < 0.001)
      return KeyValue{1, {}};
    return KeyValue{2, {}};
  };
  auto && tracing = StreetRegionsTracing{{{0.0, 0.0}, {1.0, 0.0}}, regionGetter};
  auto && segments = tracing.StealPathSegments();

  TEST_EQUAL(segments.size(), 2, ());
  TEST_EQUAL(segments.front().m_region.first, 1, ());
  TEST_EQUAL(segments.back().m_region.first, 2, ());
}

UNIT_TEST(StreetRegionTracingTest_IncomingStreet)
{
  auto regionGetter = [] (auto && point) {
    if (point.x < 0.999)
      return KeyValue{1, {}};
    return KeyValue{2, {}};
  };
  auto && tracing = StreetRegionsTracing{{{0.0, 0.0}, {1.0, 0.0}}, regionGetter};
  auto && segments = tracing.StealPathSegments();

  TEST_EQUAL(segments.size(), 2, ());
  TEST_EQUAL(segments.front().m_region.first, 1, ());
  TEST_EQUAL(segments.back().m_region.first, 2, ());
}

UNIT_TEST(StreetRegionTracingTest_StreetWithTransitRegion)
{
  auto regionGetter = [] (auto && point) {
    if (0.5 <= point.x && point.x <= 0.501)
      return KeyValue{2, {}};
    return KeyValue{1, {}};
  };
  auto && tracing = StreetRegionsTracing{{{0.0, 0.0}, {1.0, 0.0}}, regionGetter};
  auto && segments = tracing.StealPathSegments();

  TEST_EQUAL(segments.size(), 3, ());
  TEST_EQUAL(segments[0].m_region.first, 1, ());
  TEST_EQUAL(segments[1].m_region.first, 2, ());
  TEST_EQUAL(segments[2].m_region.first, 1, ());
}
