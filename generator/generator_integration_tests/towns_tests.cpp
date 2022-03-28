#include "testing/testing.hpp"

#include "generator/towns_dumper.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"

#include "base/string_utils.hpp"

#include "defines.hpp"

#include <fstream>
#include <iostream>

UNIT_TEST(MajorTowns_KansasCity)
{
  double const distanceThreshold = TownsDumper::GetDistanceThreshold();
  ms::LatLon const kansasCity{39.1001050, -94.5781416};
  auto const rect = mercator::RectByCenterXYAndSizeInMeters(mercator::FromLatLon(kansasCity), distanceThreshold);

  // Get this file from intermediate_data folder of full data generation process.
  std::ifstream in("./data/" TOWNS_FILE);

  std::string line;
  while (std::getline(in, line))
  {
    // String format: <<lat;lon;id;is_capital>>.
    std::vector const v = strings::Tokenize(line, ";");
    ms::LatLon ll;
    TEST(strings::to_double(v[0], ll.m_lat) && strings::to_double(v[1], ll.m_lon), ());

    if (rect.IsPointInside(mercator::FromLatLon(ll)) && ms::DistanceOnEarth(ll, kansasCity) < distanceThreshold)
    {
      // Kansas City was filtered by Oklahoma City.
      std::cout << v[2] << std::endl;
    }
  }
}
