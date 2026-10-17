#include "testing/testing.hpp"

#include "kml/area_data.hpp"
#include "kml/serdes.hpp"
#include "kml/serdes_binary.hpp"
#include "kml/serdes_geojson.hpp"
#include "kml/serdes_gpx.hpp"

#include "coding/writer.hpp"

#include "base/math.hpp"

namespace area_tests
{
kml::AreaRing Ring(std::initializer_list<m2::PointD> points)
{
  kml::AreaRing ring;
  for (auto const & point : points)
    ring.push_back({point, {}});
  return ring;
}

UNIT_TEST(Area_NormalizeShellAndHoles)
{
  auto const shell = Ring({{0, 0}, {10, 0}, {10, 0}, {10, 10}, {0, 10}, {0, 0}});
  auto const hole = Ring({{2, 2}, {8, 2}, {8, 8}, {2, 8}, {2, 2}});
  std::vector<kml::AreaPolygon> polygons{{shell, {hole}}};
  TEST(kml::NormalizeAreaGeometry(polygons), ());
  TEST_EQUAL(polygons.front().m_outer.size(), 4, ());
  TEST_EQUAL(polygons.front().m_holes.front().size(), 4, ());
  TEST_EQUAL(polygons.front().m_outer.front().m_point, m2::PointD(0, 0), ());
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
  polygons = {{shell, {}}};
  polygons.front().m_outer[1].m_point.x = math::Infinity();
  TEST(!kml::NormalizeAreaGeometry(polygons), ());
  polygons = {{shell, {}}};
  polygons.front().m_outer[1].m_point.y = 91;
  TEST(!kml::NormalizeAreaGeometry(polygons), ());
  polygons = {{shell, {}}};
  polygons.front().m_outer.front().m_altitude = 1;
  polygons.front().m_outer.push_back({{0, 0}, 2});
  TEST(!kml::NormalizeAreaGeometry(polygons), ());
}

UNIT_TEST(Area_IndependentPolygonsMayOverlap)
{
  std::vector<kml::AreaPolygon> polygons{{Ring({{0, 0}, {4, 0}, {4, 4}, {0, 4}}), {}},
                                         {Ring({{2, 0}, {6, 0}, {6, 4}, {2, 4}}), {}}};
  TEST(kml::NormalizeAreaGeometry(polygons), ());
  TEST_EQUAL(polygons.size(), 2, ());
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
