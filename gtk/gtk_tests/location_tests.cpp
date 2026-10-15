#include "testing/testing.hpp"

#include "gtk/location.hpp"

namespace location_tests
{
location::GpsInfo Fix(double timestamp)
{
  location::GpsInfo info;
  info.m_timestamp = timestamp;
  return info;
}

UNIT_TEST(IsCurrentLocation)
{
  double const now = 1'800'000'000;
  TEST(gtk::IsCurrentLocation(Fix(now), {}, now), ());
  TEST(gtk::IsCurrentLocation(Fix(now - 300), {}, now), ());
  // An old fix cached by GeoClue.
  TEST(!gtk::IsCurrentLocation(Fix(now - 301), {}, now), ());
  TEST(!gtk::IsCurrentLocation(Fix(now - 86400), {}, now), ());
  // The device clock may be behind the GNSS one.
  TEST(gtk::IsCurrentLocation(Fix(now + 60), {}, now), ());

  TEST(gtk::IsCurrentLocation(Fix(now), Fix(now - 1), now), ());
  TEST(gtk::IsCurrentLocation(Fix(now), Fix(now), now), ());
  // Out of order.
  TEST(!gtk::IsCurrentLocation(Fix(now - 2), Fix(now - 1), now), ());
}
}  // namespace location_tests
