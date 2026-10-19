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
  polygons = {{shell, {}}};
  polygons.front().m_outer[1].m_latLon.m_lon = math::Infinity();
  TEST(!kml::NormalizeAreaGeometry(polygons), ());
  polygons = {{shell, {}}};
  polygons.front().m_outer[1].m_latLon.m_lat = 91;
  TEST(!kml::NormalizeAreaGeometry(polygons), ());
  polygons = {{shell, {}}};
  polygons.front().m_outer[1].m_altitude = math::Infinity();
  TEST(!kml::NormalizeAreaGeometry(polygons), ());
}

UNIT_TEST(Area_IndependentPolygonsMayOverlap)
{
  std::vector<kml::AreaPolygon> polygons{{Ring({{0, 0}, {4, 0}, {4, 4}, {0, 4}}), {}},
                                         {Ring({{2, 0}, {6, 0}, {6, 4}, {2, 4}}), {}}};
  TEST(kml::NormalizeAreaGeometry(polygons), ());
  TEST_EQUAL(polygons.size(), 2, ());
}

UNIT_TEST(Area_AntimeridianShellAndHoleRetainVertices)
{
  auto shell = Ring({{179, -10}, {-179, -10}, {-179, 10}, {179, 10}});
  auto hole = Ring({{179.5, -5}, {179.5, 5}, {-179.5, 5}, {-179.5, -5}});
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
    TEST_GREATER(SignedRingArea(polygon.m_outer), 0, ());
    TEST_LESS(SignedRingArea(normalizedHole), 0, ());

    std::string text;
    MemWriter writer(text);
    kml::KmlWriter(writer).Write(data);
    TEST_EQUAL(ReadKml(text).m_areasData, data.m_areasData, (text));
  }
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

UNIT_TEST(Area_KmlClosingVertexAltitude)
{
  for (std::string const coordinates : {"0,0,10 4,0,10 0,4,10 0,0,20", "0,0 4,0 0,4 0,0,0", "0,0,0 4,0 0,4 0,0"})
  {
    auto const data =
        ReadKml("<kml><Document><Placemark><Polygon><outerBoundaryIs><LinearRing><coordinates>" + coordinates +
                "</coordinates></LinearRing></outerBoundaryIs></Polygon></Placemark>"
                "</Document></kml>");
    TEST_EQUAL(data.m_areasData.size(), 1, (coordinates));
    TEST_EQUAL(data.m_areasData.front().m_polygons.front().m_outer.size(), 3, ());
    std::string output;
    MemWriter writer(output);
    kml::KmlWriter(writer).Write(data);
    TEST_EQUAL(ReadKml(output).m_areasData, data.m_areasData, (output));
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

UNIT_TEST(Area_KmlRejectsInvalidTopology)
{
  for (std::string const polygon :
       {"<outerBoundaryIs><LinearRing><coordinates>0,0 4,4 0,4 4,0 0,0</coordinates></LinearRing></outerBoundaryIs>",
        "<outerBoundaryIs><LinearRing><coordinates>0,0 4,0 4,4 0,4 0,0</coordinates></LinearRing></outerBoundaryIs>"
        "<innerBoundaryIs><LinearRing><coordinates>5,5 6,5 6,6 5,6 5,5</coordinates></LinearRing></innerBoundaryIs>",
        "<outerBoundaryIs><LinearRing><coordinates>0,0 4,0 0,91 0,0</coordinates></LinearRing></outerBoundaryIs>"})
  {
    auto const data =
        ReadKml("<kml><Document><Placemark><Polygon>" + polygon +
                "</Polygon></Placemark>"
                "<Placemark><Polygon><outerBoundaryIs><LinearRing><coordinates>0,0 4,0 0,4 0,0</coordinates>"
                "</LinearRing></outerBoundaryIs></Polygon></Placemark></Document></kml>");
    TEST_EQUAL(data.m_areasData.size(), 1, (polygon));
    TEST_EQUAL(data.m_areasData.front().m_polygons.front().m_outer.size(), 3, ());
  }
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
