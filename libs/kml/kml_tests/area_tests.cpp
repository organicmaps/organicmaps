#include "testing/testing.hpp"

#include "kml/area_data.hpp"
#include "kml/serdes.hpp"
#include "kml/serdes_binary.hpp"
#include "kml/serdes_geojson.hpp"
#include "kml/serdes_gpx.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "base/math.hpp"

#include <algorithm>
#include <random>
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

kml::FileData ReadKml(std::string const & text)
{
  kml::FileData data;
  kml::DeserializerKml(data).Deserialize(MemReader(text.data(), text.size()));
  return data;
}

double SignedRingArea(kml::AreaRing const & ring)
{
  auto const origin = ring.front().m_latLon;
  auto previous = origin;
  double area = 0;
  for (size_t i = 1; i < ring.size(); ++i)
  {
    auto const & point = ring[i].m_latLon;
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
  // Parsers reject non-finite coordinates, see Area_*RejectsNonFiniteCoordinates tests,
  // while the range checks can't reject NaN with -ffast-math.
  for (auto const invalid : {math::Infinity(), -math::Infinity(), math::Nan()})
  {
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

UNIT_TEST(Area_KeepInvalidTopology)
{
  // Self-crossing rings and holes outside their shell are retained and interpreted by the even-odd rule.
  auto const shell = Ring({{0, 0}, {10, 0}, {10, 10}, {0, 10}});
  std::vector<kml::AreaPolygon> polygons{{Ring({{0, 0}, {4, 4}, {0, 4}, {4, 0}}), {}},
                                         {shell, {Ring({{12, 12}, {14, 12}, {14, 14}, {12, 14}})}}};
  TEST(kml::NormalizeAreaGeometry(polygons), ());
  TEST_EQUAL(polygons.size(), 2, ());
  TEST_EQUAL(polygons[0].m_outer.size(), 4, ());
  TEST_EQUAL(polygons[1].m_holes.size(), 1, ());

  // A hole touching itself at a vertex, as in ArcGIS exports.
  polygons = {{shell, {Ring({{3, 3}, {2, 4}, {2, 2}, {3, 3}, {4, 2}, {4, 4}})}}};
  TEST(kml::NormalizeAreaGeometry(polygons), ());
  TEST_EQUAL(polygons.front().m_holes.front().size(), 6, ());
  TEST_EQUAL(SignedRingArea(polygons.front().m_holes.front()), -2, ());
}

UNIT_TEST(Area_ZeroAreaRingIsStable)
{
  // Bow-ties with cancelling lobes have zero signed area or a rounding noise of it, which depends on
  // the summation order in Release builds, so their winding must not alternate.
  for (auto const & ring : {Ring({{0, 0}, {4, 4}, {0, 4}, {4, 0}}),
                            Ring({{-37.821, -32.04}, {-37.767, -32.218}, {-37.665, -32.082}, {-37.923, -32.176}})})
  {
    kml::FileData data;
    auto & polygons = data.m_areasData.emplace_back().m_polygons;
    polygons = {{ring, {}}};
    TEST(kml::NormalizeAreaGeometry(polygons), ());
    auto const normalized = polygons;
    TEST(kml::NormalizeAreaGeometry(polygons), ());
    TEST(polygons == normalized, ());

    std::string native;
    MemWriter nativeWriter(native);
    kml::KmlWriter(nativeWriter).Write(data);
    auto const stored = ReadKml(native);
    TEST_EQUAL(stored.m_areasData, data.m_areasData, (native));
    std::string json;
    MemWriter jsonWriter(json);
    kml::GeoJsonWriter(jsonWriter).Write(stored, true /* minimize */);
    kml::FileData loaded;
    kml::GeoJsonReader(loaded).Deserialize(json);
    TEST(loaded.m_areasData.front().m_polygons == normalized, (json));
  }

  // Bow-ties of parallelograms with decimal coordinates.
  std::mt19937 rng(1);
  std::uniform_int_distribution<int> coordinate(-90000, 90000);
  std::uniform_int_distribution<int> side(-300, 300);
  for (int i = 0; i < 1000; ++i)
  {
    double const x = coordinate(rng) / 1000.0, y = coordinate(rng) / 2000.0;
    double const ux = side(rng) / 1000.0, uy = side(rng) / 1000.0, vx = side(rng) / 1000.0, vy = side(rng) / 1000.0;
    std::vector<kml::AreaPolygon> polygons{
        {Ring({{x, y}, {x + ux + vx, y + uy + vy}, {x + ux, y + uy}, {x + vx, y + vy}}), {}}};
    if (!kml::NormalizeAreaGeometry(polygons))
      continue;
    auto const normalized = polygons;
    TEST(kml::NormalizeAreaGeometry(polygons), ());
    TEST(polygons == normalized, (i));
  }
}

UNIT_TEST(Area_RemoveSpikesAndDegenerateRings)
{
  std::vector<kml::AreaPolygon> polygons{
      {Ring({{0, 0}, {5, 0}, {5, -3}, {5, 0}, {10, 0}, {10, 10}, {10, 10}, {0, 10}, {-2, 12}, {0, 10}, {0, 0}}), {}}};
  TEST(kml::NormalizeAreaGeometry(polygons), ());
  TEST(polygons.front().m_outer == Ring({{0, 0}, {5, 0}, {10, 0}, {10, 10}, {0, 10}}), ());

  // Spike at the start of the ring.
  polygons = {{Ring({{-3, -3}, {0, 0}, {10, 0}, {10, 10}, {0, 10}, {0, 0}}), {}}};
  TEST(kml::NormalizeAreaGeometry(polygons), ());
  TEST(polygons.front().m_outer == Ring({{0, 0}, {10, 0}, {10, 10}, {0, 10}}), ());

  // A nested spike in any position across the start of the ring.
  auto const spiked = Ring({{0, 0}, {-1, -1}, {-2, -2}, {-1, -1}, {0, 0}, {10, 0}, {0, 10}});
  for (size_t shift = 0; shift < spiked.size(); ++shift)
  {
    auto ring = spiked;
    std::rotate(ring.begin(), ring.begin() + shift, ring.end());
    polygons = {{ring, {}}};
    TEST(kml::NormalizeAreaGeometry(polygons), (shift));
    TEST_EQUAL(polygons.front().m_outer.size(), 3, (shift));
    TEST_EQUAL(SignedRingArea(polygons.front().m_outer), 50, (shift));
  }

  // A long out-and-back chain across the start of the ring is removed in linear time.
  kml::AreaRing chain;
  size_t constexpr kChain = 40000;
  for (size_t i = kChain; i > 0; --i)
    chain.push_back({ms::LatLon(-1e-3 * i, -1e-3 * i), {}});
  for (auto const & point : Ring({{0, 0}, {10, 0}, {0, 10}, {0, 0}}))
    chain.push_back(point);
  for (size_t i = 1; i < kChain; ++i)
    chain.push_back({ms::LatLon(-1e-3 * i, -1e-3 * i), {}});
  polygons = {{chain, {}}};
  TEST(kml::NormalizeAreaGeometry(polygons), ());
  TEST_EQUAL(polygons.front().m_outer.size(), 3, ());

  // Degenerate holes and polygons are dropped, and the area is rejected only without polygons.
  auto const shell = Ring({{0, 0}, {10, 0}, {10, 10}, {0, 10}});
  polygons = {{shell, {Ring({{2, 2}, {4, 2}, {2, 2}}), Ring({{1, 1}, {2, 1}, {3, 1}})}},
              {Ring({{20, 20}, {21, 20}, {20, 20}}), {}}};
  TEST(kml::NormalizeAreaGeometry(polygons), ());
  TEST_EQUAL(polygons.size(), 1, ());
  TEST(polygons.front().m_holes.empty(), ());
  polygons = {{Ring({{0, 0}, {1, 0}, {2, 0}}), {}}};
  TEST(!kml::NormalizeAreaGeometry(polygons), ());
}

UNIT_TEST(Area_RetainAltitudeOnlyVertices)
{
  // Heights 1, 2 and 3 at the same position aren't a spike, even after removing a spike between them.
  std::pair<std::string, kml::AreaRing> const cases[] = {
      {"1,1,1 1,1,2 1,1,3 4,4,0 1,4,0 1,1,1", {{{1, 1}, 1}, {{1, 1}, 2}, {{1, 1}, 3}, {{4, 4}, 0}, {{4, 1}, 0}}},
      {"1,1,1 1,1,2 4,1,0 1,1,3 4,4,0 1,4,0 1,1,1", {{{1, 1}, 1}, {{1, 1}, 2}, {{1, 1}, 3}, {{4, 4}, 0}, {{4, 1}, 0}}},
      // The same across the start of the ring.
      {"1,1,2 1,1,3 4,4,0 1,4,0 1,1,1 1,1,2", {{{1, 1}, 2}, {{1, 1}, 3}, {{4, 4}, 0}, {{4, 1}, 0}, {{1, 1}, 1}}},
      {"4,1,0 1,1,3 4,4,0 1,4,0 1,1,1 1,1,2 4,1,0", {{{1, 1}, 3}, {{4, 4}, 0}, {{4, 1}, 0}, {{1, 1}, 1}, {{1, 1}, 2}}}};
  for (auto const & [coordinates, expected] : cases)
  {
    auto data = ReadKml("<kml><Document><Placemark><Polygon><outerBoundaryIs><LinearRing><coordinates>" + coordinates +
                        "</coordinates></LinearRing></outerBoundaryIs></Polygon></Placemark></Document></kml>");
    TEST_EQUAL(data.m_areasData.size(), 1, (coordinates));
    TEST(data.m_areasData.front().m_polygons.front().m_outer == expected, (coordinates));

    // Normalization is stable after save and load, also with heights exported to GeoJSON.
    data.m_areasData.front().m_altitudeIsWgs84 = true;
    std::string native;
    MemWriter nativeWriter(native);
    kml::KmlWriter(nativeWriter).Write(data);
    auto const stored = ReadKml(native);
    TEST_EQUAL(stored.m_areasData, data.m_areasData, (native));
    std::string json;
    MemWriter jsonWriter(json);
    kml::GeoJsonWriter(jsonWriter).Write(stored, true /* minimize */);
    kml::FileData loaded;
    kml::GeoJsonReader(loaded).Deserialize(json);
    TEST_EQUAL(loaded.m_areasData, data.m_areasData, (json));
  }
}

UNIT_TEST(Area_IndependentPolygonsMayOverlap)
{
  std::vector<kml::AreaPolygon> polygons{{Ring({{0, 0}, {4, 0}, {4, 4}, {0, 4}}), {}},
                                         {Ring({{2, 0}, {6, 0}, {6, 4}, {2, 4}}), {}}};
  TEST(kml::NormalizeAreaGeometry(polygons), ());
  TEST_EQUAL(polygons.size(), 2, ());
}

UNIT_TEST(Area_WideShellAndHoleRetainVertices)
{
  auto shell = Ring({{179, -10}, {-179, -10}, {-179, 10}, {179, 10}});
  auto hole = Ring({{-5, -5}, {-5, 5}, {5, 5}, {5, -5}});
  for (size_t i = 0; i < shell.size(); ++i)
    shell[i].m_altitude = 100.12345678901234 + i;
  hole[1].m_altitude = 0;

  for (bool reverse : {false, true})
  {
    kml::FileData data;
    auto & area = data.m_areasData.emplace_back();
    area.m_polygons.push_back({shell, {hole}});
    auto & polygon = area.m_polygons.front();
    if (reverse)
    {
      std::reverse(polygon.m_outer.begin(), polygon.m_outer.end());
      std::reverse(polygon.m_holes.front().begin(), polygon.m_holes.front().end());
    }
    polygon.m_outer.push_back(polygon.m_outer.front());
    polygon.m_holes.front().push_back(polygon.m_holes.front().front());
    TEST(kml::NormalizeAreaGeometry(area.m_polygons), (reverse));
    TEST(std::is_permutation(shell.begin(), shell.end(), polygon.m_outer.begin(), polygon.m_outer.end()), ());
    auto const & normalizedHole = polygon.m_holes.front();
    TEST(std::is_permutation(hole.begin(), hole.end(), normalizedHole.begin(), normalizedHole.end()), ());
    TEST_EQUAL(SignedRingArea(polygon.m_outer), 7160, ());
    TEST_EQUAL(SignedRingArea(normalizedHole), -100, ());

    std::string text;
    MemWriter writer(text);
    kml::KmlWriter(writer).Write(data);
    TEST_EQUAL(ReadKml(text).m_areasData, data.m_areasData, (text));
  }
}

UNIT_TEST(Area_WideShellRetainsHolesOnBothSides)
{
  auto const shell = Ring({{-170, -10}, {170, -10}, {170, 10}, {-170, 10}});
  auto const westHole = Ring({{-165, -5}, {-165, 5}, {-160, 5}, {-160, -5}});
  auto const eastHole = Ring({{160, -5}, {160, 5}, {165, 5}, {165, -5}});
  std::vector<kml::AreaPolygon> polygons{{shell, {westHole, eastHole}}};
  TEST(kml::NormalizeAreaGeometry(polygons), ());
  TEST_GREATER(SignedRingArea(polygons.front().m_outer), 0, ());
  TEST_EQUAL(polygons.front().m_holes.size(), 2, ());
  for (auto const & hole : polygons.front().m_holes)
    TEST_LESS(SignedRingArea(hole), 0, ());
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
}

UNIT_TEST(Area_GeoJsonMultiPolygonSurvivesNativeStorage)
{
  std::string const input = R"({"type":"Feature","properties":{
    "name":"Survey","description":"A & B","visibility":false,
    "fill":"#123456","fill-opacity":0.5,"stroke":"#654321","stroke-opacity":0.25,
    "stroke-width":3.5,"time":"2024-04-16T12:34:56Z"},"geometry":{
    "type":"MultiPolygon","coordinates":[
      [[[0,0,0],[10,0],[10,10],[0,10],[0,0,0]],[[2,2],[8,2],[8,8],[2,8],[2,2]]],
      [[[14.1234567890123,0,1.23456789012345],[15,0],[14,1],[14.1234567890123,0,1.23456789012345]]]
    ]}})";
  kml::FileData data;
  kml::GeoJsonReader(data).Deserialize(input);
  TEST_EQUAL(data.m_areasData.size(), 1, ());
  auto const & area = data.m_areasData.front();
  TEST(area.m_isMultiPolygon, ());
  TEST(area.m_altitudeIsWgs84, ());
  TEST_EQUAL(area.m_polygons.size(), 2, ());
  TEST_EQUAL(area.m_polygons.front().m_holes.size(), 1, ());
  TEST_EQUAL(area.m_style.m_fillColor, 0x12345680, ());
  TEST_EQUAL(area.m_style.m_outlineColor, 0x65432140, ());
  TEST_EQUAL(area.m_style.m_outlineWidth, 3.5, ());
  TEST_EQUAL(kml::GetDefaultStr(area.m_name), "Survey", ());
  TEST_EQUAL(kml::GetDefaultStr(area.m_description), "A & B", ());
  TEST_EQUAL(area.m_timestamp, kml::TimestampClock::from_time_t(base::StringToTimestamp("2024-04-16T12:34:56Z")), ());
  TEST_EQUAL(area.m_polygons[1].m_outer.front().m_latLon, ms::LatLon(0, 14.1234567890123), ());
  TEST_EQUAL(area.m_polygons[1].m_outer.front().m_altitude, std::optional<double>(1.23456789012345), ());
  TEST(!area.m_visible, ());

  for (size_t const parts : {2, 1})
  {
    data.m_areasData.front().m_polygons.resize(parts);
    std::string kml;
    MemWriter kmlWriter(kml);
    kml::KmlWriter(kmlWriter).Write(data);
    kml::FileData stored;
    kml::DeserializerKml(stored).Deserialize(MemReader(kml.data(), kml.size()));
    TEST_EQUAL(stored.m_areasData, data.m_areasData, ());
    for (bool minimize : {false, true})
    {
      std::string json;
      MemWriter jsonWriter(json);
      kml::GeoJsonWriter(jsonWriter).Write(stored, minimize);
      kml::FileData loaded;
      kml::GeoJsonReader(loaded).Deserialize(json);
      TEST_EQUAL(loaded.m_areasData, data.m_areasData, (json));
    }
  }
}

UNIT_TEST(Area_GeoJsonMixedHeightsSurviveNativeStorage)
{
  for (bool const heightInHole : {false, true})
  {
    auto const input =
        heightInHole
            ? R"({"type":"Polygon","coordinates":[[[0,0],[10,0],[10,10],[0,10],[0,0]],[[2,2,0],[2,3],[3,2],[2,2,0]]]})"
            : R"({"type":"Polygon","coordinates":[[[0,0,7],[2,0],[0,2],[0,0,7]]]})";
    kml::FileData data;
    kml::GeoJsonReader(data).Deserialize(input);
    TEST_EQUAL(data.m_areasData.size(), 1, (input));
    auto const & area = data.m_areasData.front();
    TEST(area.m_altitudeIsWgs84, (input));
    auto const & polygon = area.m_polygons.front();
    auto const & ring = heightInHole ? polygon.m_holes.front() : polygon.m_outer;
    auto expected = heightInHole ? Ring({{2, 2}, {2, 3}, {3, 2}}) : Ring({{0, 0}, {2, 0}, {0, 2}});
    expected.front().m_altitude = heightInHole ? 0 : 7;
    TEST(ring == expected, (input));
    if (heightInHole)
      for (auto const & point : polygon.m_outer)
        TEST(!point.m_altitude, (input));

    std::string native;
    MemWriter nativeWriter(native);
    kml::KmlWriter(nativeWriter).Write(data);
    auto const stored = ReadKml(native);
    TEST_EQUAL(stored.m_areasData, data.m_areasData, (native));
    for (bool const minimize : {false, true})
    {
      std::string json;
      MemWriter jsonWriter(json);
      kml::GeoJsonWriter(jsonWriter).Write(stored, minimize);
      kml::FileData loaded;
      kml::GeoJsonReader(loaded).Deserialize(json);
      TEST_EQUAL(loaded.m_areasData, data.m_areasData, (json));
    }
  }
}

UNIT_TEST(Area_GeoJsonAcceptsMatchingClosingHeights)
{
  for (std::string const ring : {R"([[0,0,10],[4,0,10],[0,4,10],[0,0,10]])", R"([[0,0],[4,0],[0,4],[0,0]])"})
  {
    kml::FileData data;
    kml::GeoJsonReader(data).Deserialize(R"({"type":"Polygon","coordinates":[)" + ring + "]}");
    TEST_EQUAL(data.m_areasData.size(), 1, (ring));
    TEST_EQUAL(data.m_areasData.front().m_polygons.size(), 1, (ring));

    std::string native;
    MemWriter nativeWriter(native);
    kml::KmlWriter(nativeWriter).Write(data);
    TEST_EQUAL(ReadKml(native).m_areasData, data.m_areasData, (native));
  }
}

UNIT_TEST(Area_GeoJsonPolygonRootsAndInvalidRing)
{
  for (
      std::string const input :
      {R"({"type":"Polygon","coordinates":[[[0,0],[2,0],[0,2],[0,0]]]})",
       R"({"type":"Feature","properties":null,"geometry":{"type":"Polygon","coordinates":[[[0,0],[2,0],[0,2],[0,0]]]}})",
       R"({"type":"FeatureCollection","features":[{"type":"Feature","properties":null,"geometry":{"type":"Polygon","coordinates":[[[0,0],[2,0],[0,2],[0,0]]]}}]})",
       R"({"features":[{"type":"Feature","properties":null,"geometry":{"type":"Polygon","coordinates":[[[0,0],[2,0],[0,2],[0,0]]]}}]})",
       R"({"features":[{"type":"Feature","properties":null,"geometry":{"type":"Polygon","coordinates":[[[0,0],[2,0],[0,2],[0,0]]]}}],"type":"FeatureCollection"})"})
  {
    kml::FileData data;
    kml::GeoJsonReader(data).Deserialize(input);
    TEST_EQUAL(data.m_areasData.size(), 1, (input));
    TEST(!data.m_areasData.front().m_isMultiPolygon, ());
    TEST(!data.m_areasData.front().m_altitudeIsWgs84, ());
  }
  std::string const invalid = R"({"type":"FeatureCollection","features":[
    {"type":"Feature","geometry":{"type":"Polygon","coordinates":[[[0,0],[1],[2,0],[0,2],[0,0]]]}},
    {"type":"Feature","geometry":{"type":"Point","coordinates":[1,1]}}
  ]})";
  kml::FileData data;
  kml::GeoJsonReader(data).Deserialize(invalid);
  TEST(data.m_areasData.empty(), ());
  TEST_EQUAL(data.m_bookmarksData.size(), 1, ());
}

UNIT_TEST(Area_GeoJsonExtraCoordinatesAndNumericTimestamp)
{
  kml::FileData data;
  kml::GeoJsonReader(data).Deserialize(R"({"type":"Feature","properties":{"time":1700000000},"geometry":{
    "type":"Polygon","coordinates":[[[0,0,10,123],[2,0,20,456],[0,2,30,789],[0,0,10,123]]]}})");
  TEST_EQUAL(data.m_areasData.size(), 1, ());
  auto const & area = data.m_areasData.front();
  TEST(area.m_altitudeIsWgs84, ());
  TEST_EQUAL(area.m_timestamp, kml::TimestampClock::from_time_t(1700000000), ());
  TEST_EQUAL(area.m_polygons.front().m_outer.size(), 3, ());
  TEST_EQUAL(area.m_polygons.front().m_outer.front().m_altitude, std::optional<double>(10), ());
}

UNIT_TEST(Area_GeoJsonNormalizesConflictingClosingHeights)
{
  std::string const shell = R"([[-1,-1],[5,-1],[5,5],[-1,5],[-1,-1]])";
  std::string const separate = R"([[-10,-10],[-8,-10],[-10,-8],[-10,-10]])";
  std::pair<std::string, kml::AreaRing> const cases[] = {
      {R"([[0,0,10],[4,0,10],[0,4,10],[0,0,20]])", {{{0, 0}, 10}, {{0, 4}, 10}, {{4, 0}, 10}}},
      {R"([[0,0],[4,0],[0,4],[0,0,0]])", Ring({{0, 0}, {4, 0}, {0, 4}})},
      {R"([[0,0,0],[4,0],[0,4],[0,0]])", {{{0, 0}, 0}, {{0, 4}, {}}, {{4, 0}, {}}}},
      {R"([[0,0,1],[4,0],[0,4],[0,0,1.0000000000000002]])", {{{0, 0}, 1}, {{0, 4}, {}}, {{4, 0}, {}}}}};
  for (auto const & [ring, expected] : cases)
  {
    std::string const geometries[] = {R"({"type":"Polygon","coordinates":[)" + ring + "]}",
                                      R"({"type":"Polygon","coordinates":[)" + shell + "," + ring + "]}",
                                      R"({"type":"MultiPolygon","coordinates":[[)" + separate + "],[" + ring + "]]}"};
    for (size_t shape = 0; auto const & geometry : geometries)
    {
      std::string const input = R"({"type":"FeatureCollection","features":[
        {"type":"Feature","geometry":)" +
                                geometry + R"(},
        {"type":"Feature","properties":{"name":"Retained"},"geometry":{
          "type":"Polygon","coordinates":[[[10,10],[12,10],[10,12],[10,10]]]}}
      ]})";
      kml::FileData data;
      kml::GeoJsonReader(data).Deserialize(input);
      TEST_EQUAL(data.m_areasData.size(), 2, (geometry));
      TEST_EQUAL(kml::GetDefaultStr(data.m_areasData.back().m_name), "Retained", (geometry));
      auto const & area = data.m_areasData.front();
      TEST_EQUAL(area.m_polygons.size(), shape == 2 ? 2 : 1, (geometry));
      TEST_EQUAL(area.m_isMultiPolygon, shape == 2, (geometry));
      TEST_EQUAL(area.m_altitudeIsWgs84, expected.front().m_altitude.has_value(), (geometry));
      auto const & polygon = area.m_polygons.back();
      TEST_EQUAL(polygon.m_holes.size(), shape == 1 ? 1 : 0, (geometry));
      auto const & normalized = shape == 1 ? polygon.m_holes.front() : polygon.m_outer;
      TEST_EQUAL(normalized.size(), expected.size(), (geometry));
      TEST(std::is_permutation(expected.begin(), expected.end(), normalized.begin(), normalized.end()), (geometry));

      std::string native;
      MemWriter writer(native);
      kml::KmlWriter(writer).Write(data);
      auto const stored = ReadKml(native);
      TEST_EQUAL(stored.m_areasData, data.m_areasData, (geometry));
      std::string json;
      MemWriter jsonWriter(json);
      kml::GeoJsonWriter(jsonWriter).Write(stored, true);
      kml::FileData exported;
      kml::GeoJsonReader(exported).Deserialize(json);
      TEST_EQUAL(exported.m_areasData, data.m_areasData, (json));
      ++shape;
    }
  }
}

UNIT_TEST(Area_GeoJsonUmapStyleSurvivesNativeStorage)
{
  kml::FileData data;
  kml::GeoJsonReader(data).Deserialize(R"({"type":"Feature","properties":{
    "fill":"#FFFFFF","stroke":"#FFFFFF","_umap_options":{
      "color":"#123456","fillColor":"#654321","opacity":0.5,"fillOpacity":0.25,
      "weight":3.5,"dashArray":"5,10"}},
    "geometry":{"type":"Polygon","coordinates":[[[0,0],[2,0],[0,2],[0,0]]]}})");
  TEST_EQUAL(data.m_areasData.size(), 1, ());
  auto const & area = data.m_areasData.front();
  TEST_EQUAL(area.m_style.m_fillColor, 0x65432140, ());
  TEST_EQUAL(area.m_style.m_outlineColor, 0x12345680, ());
  TEST_EQUAL(area.m_style.m_outlineWidth, 3.5, ());
  TEST(area.m_style.m_fill && area.m_style.m_outline, ());
  TEST(area.m_properties.contains("_umap_options"), ());

  std::string native;
  MemWriter nativeWriter(native);
  kml::KmlWriter(nativeWriter).Write(data);
  auto const stored = ReadKml(native);
  TEST_EQUAL(stored.m_areasData, data.m_areasData, ());
  std::string json;
  MemWriter jsonWriter(json);
  kml::GeoJsonWriter(jsonWriter).Write(stored, true);
  kml::FileData loaded;
  kml::GeoJsonReader(loaded).Deserialize(json);
  TEST_EQUAL(loaded.m_areasData.size(), 1, (json));
  TEST(loaded.m_areasData.front().m_style == area.m_style, (json));
  kml::geojson::GeoJsonData exported;
  TEST(!glz::read_json(exported, json), ());
  auto const & options = exported.features.front().properties->at("_umap_options").get_object();
  TEST_EQUAL(options.at("dashArray").get_string(), "5,10", ());
}

UNIT_TEST(Area_GeoJsonKeepsDistinctVerticesAtSourcePrecision)
{
  kml::FileData data;
  auto & area = data.m_areasData.emplace_back();
  area.m_polygons.push_back({Ring({{14.123456789, 0}, {14.123456790, 0}, {14.123456789, 1}}), {}});
  TEST(kml::NormalizeAreaGeometry(area.m_polygons), ());
  for (bool minimize : {false, true})
  {
    std::string json;
    MemWriter writer(json);
    kml::GeoJsonWriter(writer).Write(data, minimize);
    if (!minimize)
      TEST(json.starts_with("{\n  \"type\":"), (json));
    kml::FileData loaded;
    kml::GeoJsonReader(loaded).Deserialize(json);
    TEST_EQUAL(loaded.m_areasData, data.m_areasData, (json));
  }
}

UNIT_TEST(Area_GeoJsonMalformedPolygonKeepsOtherFeatures)
{
  for (std::string const coordinates : {R"([[[0,0],[2,null],[0,2],[0,0]]])", R"([[[0,0],[2,"invalid"],[0,2],[0,0]]])",
                                        R"([[[0,0,1],[2,0,null],[0,2,1],[0,0,1]]])", R"([[[0,0],[181,0],[0,2],[0,0]]])",
                                        R"([[[0,0],[1e300,0],[0,2],[0,0]]])"})
  {
    std::string const input = R"({"type":"FeatureCollection","features":[
      {"type":"Feature","properties":{"name":"Point"},"geometry":{"type":"Point","coordinates":[1,1]}},
      {"type":"Feature","properties":{"name":"Line","stroke":"red"},"geometry":{"type":"LineString","coordinates":[[1,1],[2,2]]}},
      {"type":"Feature","geometry":{"type":"Polygon","coordinates":)" +
                              coordinates + R"(}},
      {"type":"Feature","properties":{"name":"Area"},"geometry":{"type":"Polygon","coordinates":[[[0,0],[2,0],[0,2],[0,0]]]}}
    ]})";
    kml::FileData data;
    kml::GeoJsonReader(data).Deserialize(input);
    TEST_EQUAL(data.m_bookmarksData.size(), 1, (coordinates));
    TEST_EQUAL(data.m_tracksData.size(), 1, (coordinates));
    TEST_EQUAL(data.m_areasData.size(), 1, (coordinates));
    TEST_EQUAL(kml::GetDefaultStr(data.m_bookmarksData.front().m_name), "Point", ());
    TEST_EQUAL(kml::GetDefaultStr(data.m_tracksData.front().m_name), "Line", ());
    TEST_EQUAL(kml::GetDefaultStr(data.m_areasData.front().m_name), "Area", ());
  }
}

UNIT_TEST(Area_GeoJsonWidePolygonRetainsFootprintAndHeights)
{
  for (double const lon : {170.0, 180.0})
  {
    for (bool closed : {false, true})
    {
      kml::geojson::GeoJsonGeometryPolygon geometry;
      geometry.coordinates = {{{lon, -10, 100}, {lon, 10, 200}, {-lon, 10, 300}, {-lon, -10, 400}},
                              {{-5, -5}, {-5, 5}, {5, 5}, {5, -5}, {-5, -5}}};
      if (closed)
        geometry.coordinates.front().push_back(geometry.coordinates.front().front());
      std::string input;
      TEST(!glz::write_json(geometry, input), ());
      kml::FileData data;
      kml::GeoJsonReader(data).Deserialize(input);
      TEST_EQUAL(data.m_areasData.size(), 1, (closed));
      auto const & area = data.m_areasData.front();
      auto expected = Ring({{lon, -10}, {lon, 10}, {-lon, 10}, {-lon, -10}});
      std::vector<double> const heights = {100, 200, 300, 400};
      for (size_t i = 0; i < expected.size(); ++i)
        expected[i].m_altitude = heights[i];
      TEST(area.m_polygons.front().m_outer == expected, (input));
      TEST_EQUAL(area.m_polygons.front().m_holes.size(), 1, ());
      TEST_EQUAL(SignedRingArea(area.m_polygons.front().m_outer), 40 * lon, ());
      TEST_EQUAL(SignedRingArea(area.m_polygons.front().m_holes.front()), -100, ());

      std::string native;
      MemWriter nativeWriter(native);
      kml::KmlWriter(nativeWriter).Write(data);
      auto const stored = ReadKml(native);
      TEST_EQUAL(stored.m_areasData, data.m_areasData, ());
      std::string json;
      MemWriter jsonWriter(json);
      kml::GeoJsonWriter(jsonWriter).Write(stored, true);
      kml::FileData loaded;
      kml::GeoJsonReader(loaded).Deserialize(json);
      TEST_EQUAL(loaded.m_areasData, data.m_areasData, (json));
    }
  }
}

UNIT_TEST(Area_GeoJsonKeepsIndependentKmlPartsSeparate)
{
  kml::FileData data;
  auto & area = data.m_areasData.emplace_back();
  area.m_polygons = {{Ring({{0, 0}, {4, 0}, {4, 4}, {0, 4}}), {}}, {Ring({{2, 0}, {6, 0}, {6, 4}, {2, 4}}), {}}};
  TEST(kml::NormalizeAreaGeometry(area.m_polygons), ());
  std::string json;
  MemWriter writer(json);
  kml::GeoJsonWriter(writer).Write(data, true);
  kml::FileData loaded;
  kml::GeoJsonReader(loaded).Deserialize(json);
  TEST_EQUAL(loaded.m_areasData.size(), 2, (json));
  TEST(loaded.m_areasData[0].m_polygons.front() == area.m_polygons[0], ());
  TEST(loaded.m_areasData[1].m_polygons.front() == area.m_polygons[1], ());
}

UNIT_TEST(Area_GeoJsonDoesNotReinterpretKmlHeights)
{
  for (std::string const mode :
       {"", "clampToGround", "relativeToGround", "absolute", "clampToSeaFloor", "relativeToSeaFloor"})
    for (bool const extrude : {false, true})
    {
      kml::FileData data;
      auto & geoJsonArea = data.m_areasData.emplace_back();
      geoJsonArea.m_altitudeIsWgs84 = true;
      geoJsonArea.m_polygons.push_back({Ring({{0, 0}, {2, 0}, {0, 2}}), {}});
      geoJsonArea.m_polygons.front().m_outer.front().m_altitude = 3.25;
      auto & polygon = data.m_areasData.emplace_back().m_polygons.emplace_back();
      polygon.m_outer = Ring({{-170, -10}, {170, -10}, {170, 10}, {-170, 10}});
      polygon.m_outer.front().m_altitude = 12.5;
      polygon.m_altitudeMode = mode;
      polygon.m_extrude = extrude;
      std::string native;
      MemWriter nativeWriter(native);
      kml::KmlWriter(nativeWriter).Write(data);
      auto const stored = ReadKml(native);
      TEST_EQUAL(stored.m_areasData, data.m_areasData, (mode));
      std::string json;
      MemWriter jsonWriter(json);
      kml::GeoJsonWriter(jsonWriter).Write(stored, true);
      kml::FileData loaded;
      kml::GeoJsonReader(loaded).Deserialize(json);
      TEST_EQUAL(loaded.m_areasData.size(), 2, (mode, extrude));
      TEST_EQUAL(loaded.m_areasData.front(), data.m_areasData.front(), (json));
      for (auto const & point : loaded.m_areasData.back().m_polygons.front().m_outer)
        TEST(!point.m_altitude, (json));
      TEST(!loaded.m_areasData.back().m_altitudeIsWgs84, (json));
    }
}

UNIT_TEST(Area_GeoJsonPreservesKmlWidePolygons)
{
  // Raw wide rings and both windings match Google Earth desktop and web.
  std::vector<std::pair<std::string, double>> const cases = {{"-170,-10 170,-10 170,10 -170,10 -170,-10", 6800},
                                                             {"-170,-10 -170,10 170,10 170,-10 -170,-10", 6800},
                                                             {"-180,-10 180,-10 180,10 -180,10 -180,-10", 7200},
                                                             {"-90,-10 90,-10 90,10 -90,10 -90,-10", 3600}};
  for (auto const & [coordinates, area] : cases)
  {
    auto const data = ReadKml("<kml><Document><Placemark><Polygon><outerBoundaryIs><LinearRing><coordinates>" +
                              coordinates + R"(</coordinates></LinearRing></outerBoundaryIs>
      <innerBoundaryIs><LinearRing><coordinates>-5,-5 -5,5 5,5 5,-5 -5,-5</coordinates>
      </LinearRing></innerBoundaryIs></Polygon></Placemark></Document></kml>)");
    TEST_EQUAL(data.m_areasData.size(), 1, (coordinates));
    auto const & polygon = data.m_areasData.front().m_polygons.front();
    TEST_EQUAL(polygon.m_outer.size(), 4, (coordinates));
    TEST_EQUAL(SignedRingArea(polygon.m_outer), area, (coordinates));
    TEST_EQUAL(polygon.m_holes.size(), 1, (coordinates));
    TEST_EQUAL(SignedRingArea(polygon.m_holes.front()), -100, (coordinates));
    for (bool const minimize : {false, true})
    {
      std::string json;
      MemWriter writer(json);
      kml::GeoJsonWriter(writer).Write(data, minimize);
      kml::FileData loaded;
      kml::GeoJsonReader(loaded).Deserialize(json);
      TEST_EQUAL(loaded.m_areasData, data.m_areasData, (json));
    }
  }
}

UNIT_TEST(Area_GeoJsonSplitAntimeridianPolygonSurvivesNativeStorage)
{
  kml::FileData data;
  kml::GeoJsonReader(data).Deserialize(R"({"type":"MultiPolygon","coordinates":[
    [[[170,-10],[180,-10],[180,10],[170,10],[170,-10]]],
    [[[-180,-10],[-170,-10],[-170,10],[-180,10],[-180,-10]]]
  ]})");
  TEST_EQUAL(data.m_areasData.size(), 1, ());
  auto const & area = data.m_areasData.front();
  TEST(area.m_isMultiPolygon, ());
  TEST_EQUAL(area.m_polygons.size(), 2, ());
  for (auto const & polygon : area.m_polygons)
    TEST_EQUAL(SignedRingArea(polygon.m_outer), 200, ());

  std::string native;
  MemWriter nativeWriter(native);
  kml::KmlWriter(nativeWriter).Write(data);
  auto const stored = ReadKml(native);
  TEST_EQUAL(stored.m_areasData, data.m_areasData, (native));
  for (bool const minimize : {false, true})
  {
    std::string json;
    MemWriter writer(json);
    kml::GeoJsonWriter(writer).Write(stored, minimize);
    kml::FileData loaded;
    kml::GeoJsonReader(loaded).Deserialize(json);
    TEST_EQUAL(loaded.m_areasData, data.m_areasData, (json));
  }
}

UNIT_TEST(Area_KmlRoundTrip)
{
  kml::FileData data;
  auto & area = data.m_areasData.emplace_back();
  area.m_name[kml::kDefaultLang] = "Survey <A>";
  area.m_description[kml::kDefaultLang] = "A & B";
  area.m_style = {0, 0x11223344, 2.25, false, true};
  area.m_visible = false;
  area.m_timestamp = kml::TimestampClock::from_time_t(1234567890);
  area.m_properties["source"] = "survey & notes";
  area.m_polygons.push_back({Ring({{0, 0}, {10, 0}, {10, 10}, {0, 10}}), {Ring({{2, 2}, {8, 2}, {8, 8}, {2, 8}})}});
  area.m_polygons.push_back({Ring({{20, 0}, {21, 0}, {20, 1}}), {}});
  area.m_polygons.front().m_outer.front().m_altitude = 123.45678901234567;
  area.m_polygons.front().m_altitudeMode = "absolute";
  area.m_polygons.front().m_extrude = true;
  TEST(kml::NormalizeAreaGeometry(area.m_polygons), ());
  std::string text;
  MemWriter writer(text);
  kml::KmlWriter(writer).Write(data);
  kml::FileData loaded;
  kml::DeserializerKml(loaded).Deserialize(MemReader(text.data(), text.size()));
  TEST_EQUAL(loaded.m_areasData.size(), 1, (text));
  TEST_EQUAL(loaded.m_areasData.front(), area, (text));
  TEST(loaded.m_tracksData.empty(), ());
  TEST(loaded.m_bookmarksData.empty(), ());
}

UNIT_TEST(Area_KmlRetainsCoordinatePrecisionBeyondMercatorLimits)
{
  auto const data = ReadKml(R"(<kml><Document><Placemark><Polygon><outerBoundaryIs><LinearRing><coordinates>
    12.123456789012345,87.12345678901235,123.45678901234567
    13.123456789012345,87.12345678901235
    12.123456789012345,88.12345678901235,0
    12.123456789012345,87.12345678901235,123.45678901234567
    </coordinates></LinearRing></outerBoundaryIs></Polygon></Placemark></Document></kml>)");
  TEST_EQUAL(data.m_areasData.size(), 1, ());
  auto const & ring = data.m_areasData.front().m_polygons.front().m_outer;
  TEST_EQUAL(ring.front().m_latLon, ms::LatLon(87.12345678901235, 12.123456789012345), ());
  TEST_EQUAL(*ring.front().m_altitude, 123.45678901234567, ());
  TEST(!ring[1].m_altitude, ());
  TEST_EQUAL(*ring[2].m_altitude, 0, ());
  std::string output;
  MemWriter writer(output);
  kml::KmlWriter(writer).Write(data);
  TEST_EQUAL(ReadKml(output).m_areasData, data.m_areasData, (output));
}

UNIT_TEST(Area_KmlMixedGeometryAndForwardStyle)
{
  std::string const text = R"(<kml><Document>
    <Placemark><name>Mixed</name><styleUrl>#alias</styleUrl>
      <Style><PolyStyle><outline>false</outline></PolyStyle></Style>
      <MultiGeometry>
        <Point><coordinates>1,1</coordinates></Point>
        <LineString><coordinates>1,1 2,2</coordinates></LineString>
        <MultiGeometry><Polygon><outerBoundaryIs><LinearRing>
          <coordinates>0,0 4,0 0,4 0,0</coordinates>
        </LinearRing></outerBoundaryIs></Polygon></MultiGeometry>
        <Polygon><outerBoundaryIs><LinearRing>
          <coordinates>10,0 14,0 10,4 10,0</coordinates>
        </LinearRing></outerBoundaryIs></Polygon>
      </MultiGeometry>
    </Placemark>
    <StyleMap id="alias"><Pair><key>normal</key><styleUrl>#shared</styleUrl></Pair></StyleMap>
    <Style id="shared"><LineStyle><color>44332211</color><width>3</width></LineStyle>
      <PolyStyle><color>800000ff</color><fill>false</fill></PolyStyle>
    </Style>
  </Document></kml>)";
  kml::FileData data;
  kml::DeserializerKml(data).Deserialize(MemReader(text.data(), text.size()));
  TEST_EQUAL(data.m_areasData.size(), 1, ());
  auto const & area = data.m_areasData.front();
  TEST_EQUAL(area.m_polygons.size(), 2, ());
  TEST_EQUAL(area.m_name.at(kml::kDefaultLang), "Mixed", ());
  TEST_EQUAL(area.m_style.m_fillColor, 0xFF000080, ());
  TEST_EQUAL(area.m_style.m_outlineColor, 0x11223344, ());
  TEST_EQUAL(area.m_style.m_outlineWidth, 3, ());
  TEST(!area.m_style.m_fill && !area.m_style.m_outline, ());
  TEST_EQUAL(data.m_bookmarksData.size(), 1, ());
  TEST_EQUAL(data.m_tracksData.size(), 1, ());
}

UNIT_TEST(Area_KmlRejectsBrokenRingWithoutJoiningAcrossBadCoordinates)
{
  std::string const text = R"(<kml><Document><Placemark><Polygon>
    <outerBoundaryIs><LinearRing><coordinates>0,0 4,0 broken 0,4 0,0</coordinates>
    </LinearRing></outerBoundaryIs></Polygon></Placemark>
    <Placemark><Point><coordinates>1,1</coordinates></Point></Placemark>
    </Document></kml>)";
  kml::FileData data;
  kml::DeserializerKml(data).Deserialize(MemReader(text.data(), text.size()));
  TEST(data.m_areasData.empty(), ());
  TEST_EQUAL(data.m_bookmarksData.size(), 1, ());
}

UNIT_TEST(Area_KmlNormalizesConflictingClosingHeights)
{
  std::pair<std::string, kml::AreaRing> const cases[] = {
      {"0,0,10 4,0,10 0,4,10 0,0,20", {{{0, 0}, 10}, {{0, 4}, 10}, {{4, 0}, 10}}},
      {"0,0 4,0 0,4 0,0,0", Ring({{0, 0}, {4, 0}, {0, 4}})},
      {"0,0,0 4,0 0,4 0,0", {{{0, 0}, 0}, {{0, 4}, {}}, {{4, 0}, {}}}},
      {"0,0,1 4,0 0,4 0,0,1.0000000000000002", {{{0, 0}, 1}, {{0, 4}, {}}, {{4, 0}, {}}}}};
  for (auto const & [coordinates, expected] : cases)
  {
    auto const data =
        ReadKml("<kml><Document><Placemark><Polygon><outerBoundaryIs><LinearRing><coordinates>" + coordinates +
                "</coordinates></LinearRing></outerBoundaryIs></Polygon></Placemark>"
                "<Placemark><name>Retained</name><Polygon><outerBoundaryIs><LinearRing>"
                "<coordinates>10,10 12,10 10,12 10,10</coordinates>"
                "</LinearRing></outerBoundaryIs></Polygon></Placemark>"
                "</Document></kml>");
    TEST_EQUAL(data.m_areasData.size(), 2, (coordinates));
    TEST_EQUAL(kml::GetDefaultStr(data.m_areasData.back().m_name), "Retained", (coordinates));
    TEST_EQUAL(data.m_areasData.front().m_polygons.size(), 1, (coordinates));
    auto const & ring = data.m_areasData.front().m_polygons.front().m_outer;
    TEST(ring == expected, (coordinates));
    std::string native;
    MemWriter writer(native);
    kml::KmlWriter(writer).Write(data);
    TEST_EQUAL(ReadKml(native).m_areasData, data.m_areasData, (coordinates));
  }
}

UNIT_TEST(Area_KmlAcceptsMatchingClosingHeights)
{
  for (std::string const coordinates : {"0,0,10 4,0,10 0,4,10 0,0,10", "0,0 4,0 0,4 0,0"})
  {
    auto const data =
        ReadKml("<kml><Document><Placemark><Polygon><outerBoundaryIs><LinearRing><coordinates>" + coordinates +
                "</coordinates></LinearRing></outerBoundaryIs></Polygon></Placemark></Document></kml>");
    TEST_EQUAL(data.m_areasData.size(), 1, (coordinates));
    TEST_EQUAL(data.m_areasData.front().m_polygons.size(), 1, (coordinates));
    TEST_EQUAL(data.m_areasData.front().m_polygons.front().m_outer.size(), 3, ());
    std::string output;
    MemWriter writer(output);
    kml::KmlWriter(writer).Write(data);
    TEST_EQUAL(ReadKml(output).m_areasData, data.m_areasData, (output));
  }
}

UNIT_TEST(Area_KmlRejectsNonFiniteCoordinates)
{
  for (std::string const tuple :
       {"NaN,0", "0,NaN", "0,0,NaN", "INF,0", "0,-INF", "0,0,INF", "1e309,0", "0,1e309", "0,0,1e309"})
  {
    auto const data =
        ReadKml("<kml><Document><Placemark><Polygon><outerBoundaryIs><LinearRing><coordinates>0,0 " + tuple +
                " 4,0 0,4 0,0</coordinates></LinearRing></outerBoundaryIs>"
                "</Polygon></Placemark></Document></kml>");
    TEST(data.m_areasData.empty(), (tuple));
  }
}

UNIT_TEST(Area_GeoJsonEmptyPartsAreDropped)
{
  kml::FileData data;
  kml::GeoJsonReader(data).Deserialize(R"({"type":"FeatureCollection","features":[
    {"type":"Feature","geometry":{"type":"MultiPolygon","coordinates":[[],[[[0,0],[4,0],[0,4],[0,0]]],[[]]]}},
    {"type":"Feature","geometry":{"type":"Polygon","coordinates":[]}}
  ]})");
  TEST_EQUAL(data.m_areasData.size(), 1, ());
  TEST_EQUAL(data.m_areasData.front().m_polygons.size(), 1, ());
}

UNIT_TEST(Area_GeoJsonRejectsNonFiniteCoordinates)
{
  for (std::string const tuple :
       {"NaN,0", "0,NaN", "0,0,NaN", "Infinity,0", "0,-Infinity", "0,0,Infinity", "1e309,0", "0,1e309", "0,0,1e309"})
  {
    kml::FileData data;
    kml::GeoJsonReader(data).Deserialize(R"({"type":"FeatureCollection","features":[
      {"type":"Feature","geometry":{"type":"Polygon","coordinates":[[[0,0],[)" +
                                         tuple + R"(],[4,0],[0,4],[0,0]]]}},
      {"type":"Feature","geometry":{"type":"Polygon","coordinates":[[[10,10],[12,10],[10,12],[10,10]]]}}
    ]})");
    TEST_EQUAL(data.m_areasData.size(), 1, (tuple));
  }
}

UNIT_TEST(Area_KmlInvalidOutlineWidthsKeepSharedStyle)
{
  for (std::string const width : {"NaN", "INF", "-INF", "1e309", "-1"})
  {
    auto const data = ReadKml(
        "<kml><Document><Style id=\"shared\"><LineStyle><width>4</width></LineStyle></Style>"
        "<Placemark><styleUrl>#shared</styleUrl><Style><LineStyle><width>" +
        width +
        "</width></LineStyle></Style><Polygon><outerBoundaryIs><LinearRing>"
        "<coordinates>0,0 4,0 0,4 0,0</coordinates></LinearRing></outerBoundaryIs>"
        "</Polygon></Placemark></Document></kml>");
    TEST_EQUAL(data.m_areasData.size(), 1, (width));
    TEST_EQUAL(data.m_areasData.front().m_style.m_outlineWidth, 4, (width));
  }
}

UNIT_TEST(Area_KmlMultipleInnerRings)
{
  auto const data = ReadKml(R"(<kml><Document><Placemark><Polygon>
    <outerBoundaryIs><LinearRing><coordinates>0,0 10,0 10,10 0,10 0,0</coordinates></LinearRing></outerBoundaryIs>
    <innerBoundaryIs>
      <LinearRing><coordinates>1,1 2,1 2,2 1,2 1,1</coordinates></LinearRing>
      <LinearRing><coordinates>3,3 4,3 4,4 3,4 3,3</coordinates></LinearRing>
    </innerBoundaryIs>
    <innerBoundaryIs><LinearRing><coordinates>5,5 6,5 6,6 5,6 5,5</coordinates></LinearRing></innerBoundaryIs>
    </Polygon></Placemark></Document></kml>)");
  TEST_EQUAL(data.m_areasData.size(), 1, ());
  TEST_EQUAL(data.m_areasData.front().m_polygons.front().m_holes.size(), 3, ());
  std::string output;
  MemWriter writer(output);
  kml::KmlWriter(writer).Write(data);
  TEST_EQUAL(ReadKml(output).m_areasData, data.m_areasData, (output));
}

UNIT_TEST(Area_KmlKeepsInvalidTopologyAndRejectsBadCoordinates)
{
  auto const read = [](std::string const & polygon)
  {
    return ReadKml("<kml><Document><Placemark><Polygon>" + polygon +
                   "</Polygon></Placemark>"
                   "<Placemark><Polygon><outerBoundaryIs><LinearRing><coordinates>0,0 4,0 0,4 0,0</coordinates>"
                   "</LinearRing></outerBoundaryIs></Polygon></Placemark></Document></kml>");
  };

  // A self-crossing ring and a hole outside its shell are retained.
  auto data = read(
      "<outerBoundaryIs><LinearRing><coordinates>0,0 4,4 0,4 4,0 0,0</coordinates></LinearRing></outerBoundaryIs>");
  TEST_EQUAL(data.m_areasData.size(), 2, ());
  TEST(data.m_areasData.front().m_polygons.front().m_outer == Ring({{0, 0}, {4, 4}, {0, 4}, {4, 0}}), ());
  data = read(
      "<outerBoundaryIs><LinearRing><coordinates>0,0 4,0 4,4 0,4 0,0</coordinates></LinearRing></outerBoundaryIs>"
      "<innerBoundaryIs><LinearRing><coordinates>5,5 6,5 6,6 5,6 5,5</coordinates></LinearRing></innerBoundaryIs>");
  TEST_EQUAL(data.m_areasData.size(), 2, ());
  TEST_EQUAL(data.m_areasData.front().m_polygons.front().m_holes.size(), 1, ());

  // Coordinates out of range reject the area.
  data =
      read("<outerBoundaryIs><LinearRing><coordinates>0,0 4,0 0,91 0,0</coordinates></LinearRing></outerBoundaryIs>");
  TEST_EQUAL(data.m_areasData.size(), 1, ());
  TEST(data.m_areasData.front().m_polygons.front().m_outer == Ring({{0, 0}, {4, 0}, {0, 4}}), ());
}

UNIT_TEST(Area_KmlCyclicAndMissingStylesKeepInlineOverrides)
{
  for (std::string const reference : {"#first", "#missing", "https://example.org/styles.kml#style"})
  {
    auto const data = ReadKml("<kml><Document><Placemark><styleUrl>" + reference + R"(</styleUrl>
      <Style><PolyStyle><fill>0</fill></PolyStyle></Style>
      <Polygon><outerBoundaryIs><LinearRing><coordinates>0,0 4,0 0,4 0,0</coordinates>
      </LinearRing></outerBoundaryIs></Polygon></Placemark>
      <StyleMap id="first"><Pair><key>normal</key><styleUrl>#second</styleUrl></Pair></StyleMap>
      <StyleMap id="second"><Pair><key>normal</key><styleUrl>#first</styleUrl></Pair></StyleMap>
      </Document></kml>)");
    TEST_EQUAL(data.m_areasData.size(), 1, (reference));
    kml::AreaStyle expected;
    expected.m_fill = false;
    TEST(data.m_areasData.front().m_style == expected, (reference));
  }
}

UNIT_TEST(Area_KmlStyleMapScopesAndInlineOverrides)
{
  std::string const styles = R"(
    <StyleMap id="map">
      <Pair><key>normal</key><Style>
        <LineStyle><color>80332211</color><width>4</width></LineStyle>
        <PolyStyle><color>800000ff</color><fill>0</fill><outline>0</outline></PolyStyle>
      </Style></Pair>
      <Pair><key>highlight</key><Style><PolyStyle><color>8000ff00</color><fill>1</fill></PolyStyle></Style></Pair>
    </StyleMap>
    <StyleMap id="alias"><Pair><key>normal</key><styleUrl>#map</styleUrl></Pair></StyleMap>)";
  std::string const polygon = R"(<Polygon><outerBoundaryIs><LinearRing>
    <coordinates>0,0 4,0 4,4 0,4 0,0</coordinates>
    </LinearRing></outerBoundaryIs></Polygon>)";
  std::string const placemarks = "<Placemark><styleUrl>#alias</styleUrl>" + polygon + "</Placemark>" +
                                 "<Placemark><styleUrl>#map</styleUrl>" + polygon + "</Placemark>" + "<Placemark>" +
                                 polygon + "</Placemark>" +
                                 "<Placemark><styleUrl>#map</styleUrl><Style><LineStyle><width>7</width></LineStyle>"
                                 "<PolyStyle><fill>1</fill></PolyStyle></Style>" +
                                 polygon + "</Placemark>";
  for (bool forwardReference : {false, true})
  {
    auto const data = ReadKml("<kml><Document>" + (forwardReference ? placemarks + styles : styles + placemarks) +
                              "</Document></kml>");
    TEST_EQUAL(data.m_areasData.size(), 4, ());
    for (size_t i : {0, 1, 3})
    {
      auto const & style = data.m_areasData[i].m_style;
      TEST_EQUAL(style.m_fillColor, 0xFF000080, (i));
      TEST_EQUAL(style.m_outlineColor, 0x11223380, (i));
      TEST(!style.m_outline, (i));
      TEST_EQUAL(style.m_fill, i == 3, ());
      TEST_EQUAL(style.m_outlineWidth, i == 3 ? 7 : 4, ());
    }
    TEST(data.m_areasData[2].m_style == kml::AreaStyle{}, ());
  }
}

UNIT_TEST(Area_KmlInlineStyleSelectors)
{
  std::string const polygon = R"(<Polygon><outerBoundaryIs><LinearRing>
    <coordinates>0,0 4,0 0,4 0,0</coordinates>
    </LinearRing></outerBoundaryIs></Polygon>)";
  auto const data = ReadKml(R"(<kml><Document>
    <Placemark><StyleMap>
      <Pair><key>normal</key><Style><PolyStyle><color>800000ff</color><fill>0</fill></PolyStyle></Style></Pair>
      <Pair><key>highlight</key><Style><PolyStyle><color>8000ff00</color><fill>1</fill></PolyStyle></Style></Pair>
    </StyleMap>)" + polygon +
                            R"(</Placemark>
    <Placemark><StyleMap><Pair><key>normal</key><styleUrl>#normal</styleUrl></Pair></StyleMap>)" +
                            polygon +
                            R"(</Placemark><Placemark>
      <Style><LineStyle><color>80332211</color><width>4</width></LineStyle></Style>
      <Style><PolyStyle><fill>0</fill></PolyStyle></Style>)" +
                            polygon + R"(</Placemark>
    <Style id="normal"><PolyStyle><color>800000ff</color><fill>0</fill></PolyStyle></Style>
    </Document></kml>)");
  TEST_EQUAL(data.m_areasData.size(), 3, ());
  for (size_t i : {0, 1})
  {
    TEST_EQUAL(data.m_areasData[i].m_style.m_fillColor, 0xFF000080, ());
    TEST(!data.m_areasData[i].m_style.m_fill, ());
  }
  auto const & style = data.m_areasData[2].m_style;
  TEST_EQUAL(style.m_outlineColor, 0x11223380, ());
  TEST_EQUAL(style.m_outlineWidth, 4, ());
  TEST(!style.m_fill, ());
}

UNIT_TEST(Area_KmlAltitudeModesAndElementOrder)
{
  for (std::string const mode :
       {"clampToGround", "relativeToGround", "absolute", "clampToSeaFloor", "relativeToSeaFloor"})
  {
    auto const tag = mode == "clampToSeaFloor" || mode == "relativeToSeaFloor" ? "gx:altitudeMode" : "altitudeMode";
    auto const data = ReadKml(
        "<kml xmlns:gx=\"http://www.google.com/kml/ext/2.2\"><Document><Placemark>"
        "<name>Survey</name><visibility>0</visibility><description>Elevated area</description>"
        "<Polygon><extrude>true</extrude><" +
        std::string(tag) + ">" + mode + "</" + tag +
        ">"
        "<outerBoundaryIs><LinearRing><coordinates>0,0,100.123456789 4,0,100 0,4,100 0,0,100.123456789</coordinates>"
        "</LinearRing></outerBoundaryIs></Polygon></Placemark></Document></kml>");
    TEST_EQUAL(data.m_areasData.size(), 1, (mode));
    auto const & polygon = data.m_areasData.front().m_polygons.front();
    TEST_EQUAL(polygon.m_altitudeMode, mode, ());
    TEST(polygon.m_extrude, ());

    std::string text;
    MemWriter writer(text);
    kml::KmlWriter(writer).Write(data);
    TEST_EQUAL(ReadKml(text).m_areasData, data.m_areasData, (text));
    auto const placemark = text.substr(text.find("<Placemark>"));
    TEST_LESS(placemark.find("<name>"), placemark.find("<visibility>"), ());
    TEST_LESS(placemark.find("<visibility>"), placemark.find("<description>"), ());
    TEST_LESS(placemark.find("<Style>"), placemark.find("<ExtendedData"), ());
    TEST_LESS(placemark.find("<ExtendedData"), placemark.find("<Polygon>"), ());
    TEST_LESS(placemark.find("<extrude>"), placemark.find("<" + std::string(tag) + ">"), ());
    TEST_NOT_EQUAL(placemark.find("<" + std::string(tag) + ">" + mode + "</" + tag + ">"), std::string::npos, ());
  }
}
}  // namespace area_tests
