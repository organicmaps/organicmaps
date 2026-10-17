#include "testing/testing.hpp"

#include "kml/area_data.hpp"
#include "kml/serdes.hpp"
#include "kml/serdes_binary.hpp"
#include "kml/serdes_geojson.hpp"
#include "kml/serdes_gpx.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "geometry/mercator.hpp"

#include "base/math.hpp"

#include <algorithm>
#include <utility>

namespace area_tests
{
kml::AreaRing Ring(std::initializer_list<std::pair<double, double>> lonLat)
{
  kml::AreaRing ring;
  for (auto const & [lon, lat] : lonLat)
    ring.push_back({ms::LatLon(lat, lon), {}});
  return ring;
}

double SignedRingArea(kml::AreaRing const & ring)
{
  auto const origin = ring.front().m_latLon;
  auto previous = origin;
  double area = 0;
  for (size_t i = 1; i < ring.size(); ++i)
  {
    auto point = ring[i].m_latLon;
    point.m_lon = mercator::NearestWrapX(point.m_lon, previous.m_lon);
    area += (previous.m_lon - origin.m_lon) * (point.m_lat - origin.m_lat) -
            (previous.m_lat - origin.m_lat) * (point.m_lon - origin.m_lon);
    previous = point;
  }
  return area / 2;
}

UNIT_TEST(Area_NormalizeShellAndHoles)
{
  auto const shell = Ring({{0, 0}, {10, 0}, {10, 0}, {10, 10}, {0, 10}, {0, 0}});
  auto const hole = Ring({{2, 2}, {8, 2}, {8, 8}, {2, 8}, {2, 2}});
  std::vector<kml::AreaPolygon> polygons{{shell, {hole}}};
  TEST(kml::NormalizeAreaGeometry(polygons), ());
  TEST_EQUAL(polygons.front().m_outer.size(), 4, ());
  TEST_EQUAL(polygons.front().m_holes.front().size(), 4, ());
  TEST_EQUAL(polygons.front().m_outer.front().m_latLon, ms::LatLon(0, 0), ());
}

UNIT_TEST(Area_RejectInvalidGeometry)
{
  auto const shell = Ring({{0, 0}, {10, 0}, {10, 10}, {0, 10}});
  std::vector<kml::AreaPolygon> polygons;
  TEST(!kml::NormalizeAreaGeometry(polygons), ());
  polygons = {{Ring({{0, 0}, {4, 4}, {0, 4}, {4, 0}}), {}}};
  TEST(!kml::NormalizeAreaGeometry(polygons), ());
  polygons = {{shell, {Ring({{12, 12}, {14, 12}, {14, 14}, {12, 14}})}}};
  TEST(!kml::NormalizeAreaGeometry(polygons), ());
  for (auto const invalid : {math::Infinity(), -math::Infinity(), math::Nan()})
  {
    polygons = {{shell, {}}};
    polygons.front().m_outer[1].m_latLon.m_lon = invalid;
    TEST(!kml::NormalizeAreaGeometry(polygons), (invalid));
    polygons = {{shell, {}}};
    polygons.front().m_outer[1].m_latLon.m_lat = invalid;
    TEST(!kml::NormalizeAreaGeometry(polygons), (invalid));
    polygons = {{shell, {}}};
    polygons.front().m_outer[1].m_altitude = invalid;
    TEST(!kml::NormalizeAreaGeometry(polygons), (invalid));
  }
  polygons = {{shell, {}}};
  polygons.front().m_outer[1].m_latLon.m_lat = 91;
  TEST(!kml::NormalizeAreaGeometry(polygons), ());
  polygons = {{shell, {}}};
  polygons.front().m_outer[1].m_latLon.m_lon = 181;
  TEST(!kml::NormalizeAreaGeometry(polygons), ());
}

UNIT_TEST(Area_IndependentPolygonsMayOverlap)
{
  std::vector<kml::AreaPolygon> polygons{{Ring({{0, 0}, {4, 0}, {4, 4}, {0, 4}}), {}},
                                         {Ring({{2, 0}, {6, 0}, {6, 4}, {2, 4}}), {}}};
  TEST(kml::NormalizeAreaGeometry(polygons), ());
  TEST_EQUAL(polygons.size(), 2, ());
}

UNIT_TEST(Area_WideShellRetainsHolesOnBothSides)
{
  auto const shell = Ring({{-170, -10}, {0, -10}, {170, -10}, {170, 10}, {0, 10}, {-170, 10}});
  auto const westHole = Ring({{-165, -5}, {-165, 5}, {-160, 5}, {-160, -5}});
  auto const eastHole = Ring({{160, -5}, {160, 5}, {165, 5}, {165, -5}});
  std::vector<kml::AreaPolygon> polygons{{shell, {westHole, eastHole}}};
  TEST(kml::NormalizeAreaGeometry(polygons), ());
  TEST_GREATER(SignedRingArea(polygons.front().m_outer), 0, ());
  TEST_EQUAL(polygons.front().m_holes.size(), 2, ());
  for (auto const & hole : polygons.front().m_holes)
    TEST_LESS(SignedRingArea(hole), 0, ());
}

UNIT_TEST(Area_RejectInvalidAntimeridianTopology)
{
  auto const shell = Ring({{179, -10}, {-179, -10}, {-179, 10}, {179, 10}});
  std::vector<kml::AreaPolygon> polygons{{shell, {Ring({{179.5, 12}, {-179.5, 12}, {-179.5, 14}, {179.5, 14}})}}};
  TEST(!kml::NormalizeAreaGeometry(polygons), ());
  polygons = {{Ring({{179, -10}, {-179, 10}, {179, 10}, {-179, -10}}), {}}};
  TEST(!kml::NormalizeAreaGeometry(polygons), ());
}

UNIT_TEST(Area_UnsupportedFormatsDoNotWritePartialOutput)
{
  kml::FileData data;
  data.m_areasData.emplace_back().m_polygons.push_back({Ring({{0, 0}, {1, 0}, {0, 1}}), {}});
  TEST(data.HasContent(), ());
  std::string output;
  MemWriter writer(output);
  TEST_ANY_THROW(kml::binary::SerializerKml(data).Serialize(writer), ());
  TEST(output.empty(), ());
  TEST_ANY_THROW(kml::gpx::GpxWriter(writer).Write(data), ());
  TEST(output.empty(), ());
  TEST_ANY_THROW(kml::GeoJsonWriter(writer).Write(data, false), ());
  TEST(output.empty(), ());
  TEST_ANY_THROW(kml::KmlWriter(writer).Write(data), ());
  TEST(output.empty(), ());
}

}  // namespace area_tests
