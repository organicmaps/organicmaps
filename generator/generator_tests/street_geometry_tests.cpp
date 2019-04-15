#include "testing/testing.hpp"

#include "generator/streets/street_geometry.hpp"

#include "geometry/point2d.hpp"

using namespace generator::streets;

UNIT_TEST(HighwayGeometryTest_PinAtSingleLineCenter)
{
  HighwayGeometry highway{};
  highway.AddLine(base::MakeOsmWay(1), {{0.0, 0.0}, {1.0, 1.0}});

  auto pin = highway.ChoosePin();
  TEST_EQUAL(pin.m_osmId, base::MakeOsmWay(1), ());
  TEST(m2::AlmostEqualAbs(pin.m_position, {0.5, 0.5}, 0.001), ());
}

UNIT_TEST(HighwayGeometryTest_PinAtHalfWaySegmentCenter)
{
  HighwayGeometry highway{};
  highway.AddLine(base::MakeOsmWay(1), {{0.0, 0.0}, {1.0, 0.0}});
  highway.AddLine(base::MakeOsmWay(3), {{1.5, 0.5}, {2.0, 0.5}});
  highway.AddLine(base::MakeOsmWay(5), {{2.5, 0.0}, {3.8, 0.0}});
  highway.AddLine(base::MakeOsmWay(2), {{1.0, 0.0}, {1.5, 0.5}});
  highway.AddLine(base::MakeOsmWay(4), {{2.0, 0.5}, {2.5, 0.0}});

  auto pin = highway.ChoosePin();
  TEST_EQUAL(pin.m_osmId, base::MakeOsmWay(3), ());
  TEST(m2::AlmostEqualAbs(pin.m_position, {1.75, 0.5}, 0.001), ());
}
