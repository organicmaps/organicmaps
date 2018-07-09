#include "testing/testing.hpp"

#include "map/ge0_parser.hpp"
#include "map/mwm_url.hpp"

#include "api/internal/c/api-client-internals.h"
#include "api/src/c/api-client.h"

#include "base/macros.hpp"


using url_scheme::Ge0Parser;
using url_scheme::ApiPoint;

namespace
{
class Ge0ParserForTest : public Ge0Parser
{
public:
  using Ge0Parser::DecodeBase64Char;
  using Ge0Parser::DecodeZoom;
  using Ge0Parser::DecodeLatLon;
};

double GetLatEpsilon(size_t coordBytes)
{
  // Should be / 2.0 but probably because of accumulates loss of precision, 1.77 works but 2.0 doesn't.
  double infelicity = 1 << ((MAPSWITHME_MAX_POINT_BYTES - coordBytes) * 3);
  return infelicity / ((1 << MAPSWITHME_MAX_COORD_BITS) - 1) * 180 / 1.77;
}

double GetLonEpsilon(size_t coordBytes)
{
  // Should be / 2.0 but probably because of accumulates loss of precision, 1.77 works but 2.0 doesn't.
  double infelicity = 1 << ((MAPSWITHME_MAX_POINT_BYTES - coordBytes) * 3);
  return (infelicity / ((1 << MAPSWITHME_MAX_COORD_BITS) - 1)) * 360 / 1.77;
}

void TestSuccess(char const * s, double lat, double lon, double zoom, char const * name)
{
  Ge0Parser parser;
  ApiPoint apiPoint;
  double parsedZoomLevel;
  bool const result = parser.Parse(s, apiPoint, parsedZoomLevel);

  TEST(result, (s, zoom, lat, lon, name));

  TEST_EQUAL(apiPoint.m_name, string(name), (s));
  TEST_EQUAL(apiPoint.m_id, string(), (s));
  double const latEps = GetLatEpsilon(9);
  double const lonEps = GetLonEpsilon(9);
  TEST(fabs(apiPoint.m_lat - lat) <= latEps, (s, zoom, lat, lon, name));
  TEST(fabs(apiPoint.m_lon - lon) <= lonEps, (s, zoom, lat, lon, name));

  TEST(fabs(apiPoint.m_lat - lat) <= latEps, (s, zoom, lat, lon, name));
  TEST(fabs(apiPoint.m_lon - lon) <= lonEps, (s, zoom, lat, lon, name));
  TEST_ALMOST_EQUAL_ULPS(parsedZoomLevel, zoom, (s, zoom, lat, lon, name));
}

void TestFailure(char const * s)
{
  Ge0Parser parser;
  ApiPoint apiPoint;
  double zoomLevel;
  bool const result = parser.Parse(s, apiPoint, zoomLevel);
  TEST_EQUAL(result, false, (s));
}

bool ConvergenceTest(double lat, double lon, double latEps, double lonEps)
{
  double tmpLat = lat, tmpLon = lon;
  Ge0ParserForTest parser;
  for (size_t i = 0; i < 100000; ++i)
  {
    char urlPrefix[] = "Coord6789";
    MapsWithMe_LatLonToString(tmpLat, tmpLon, urlPrefix + 0, 9);
    parser.DecodeLatLon(urlPrefix, tmpLat, tmpLon);
  }
  if (fabs(lat - tmpLat) <= latEps && fabs(lon - tmpLon) <= lonEps)
    return true;
  return false;
}
}  // namespace

UNIT_TEST(Base64DecodingWorksForAValidChar)
{
  Ge0ParserForTest parser;
  for (int i = 0; i < 64; ++i)
  {
    char c = MapsWithMe_Base64Char(i);
    int i1 = parser.DecodeBase64Char(c);
    TEST_EQUAL(i, i1, (c));
  }
}

UNIT_TEST(Base64DecodingReturns255ForInvalidChar)
{
  Ge0ParserForTest parser;
  TEST_EQUAL(parser.DecodeBase64Char(' '), 255, ());
}

UNIT_TEST(Base64DecodingDoesNotCrashForAllChars)
{
  Ge0ParserForTest parser;
  for (size_t i = 0; i < 256; ++i)
    parser.DecodeBase64Char(static_cast<char>(i));
}

UNIT_TEST(Base64DecodingCharFrequency)
{
  vector<int> charCounts(256, 0);
  Ge0ParserForTest parser;
  for (size_t i = 0; i < 256; ++i)
    ++charCounts[parser.DecodeBase64Char(static_cast<char>(i))];
  sort(charCounts.begin(), charCounts.end());
  TEST_EQUAL(charCounts[255], 256 - 64, ());
  TEST_EQUAL(charCounts[254], 1, ());
  TEST_EQUAL(charCounts[254 - 63], 1, ());
  TEST_EQUAL(charCounts[254 - 64], 0, ());
  TEST_EQUAL(charCounts[0], 0, ());
}

UNIT_TEST(UrlSchemaValidationFailed)
{
  TestFailure("trali vali");
  TestFailure("trali vali tili tili eto my prohodili");
}

UNIT_TEST(DecodeZoomLevel)
{
  TEST_EQUAL(Ge0ParserForTest::DecodeZoom(0), 4, ());
  TEST_EQUAL(Ge0ParserForTest::DecodeZoom(4), 5, ());
  TEST_EQUAL(Ge0ParserForTest::DecodeZoom(6), 5.5, ());
  TEST_EQUAL(Ge0ParserForTest::DecodeZoom(53), 17.25, ());
  TEST_EQUAL(Ge0ParserForTest::DecodeZoom(60), 19, ());
  TEST_EQUAL(Ge0ParserForTest::DecodeZoom(63), 19.75, ());
  TestFailure("ge0://!wAAAAAAAA/Name");
  TestFailure("ge0:///wAAAAAAAA/Name");
}

UNIT_TEST(LatLonConvergence)
{
  double const latEps = GetLatEpsilon(9);
  double const lonEps = GetLonEpsilon(9);
  TEST(ConvergenceTest(0, 0, latEps, lonEps), ());
  TEST(ConvergenceTest(1.111111, 2.11111, latEps, lonEps), ());
  TEST(ConvergenceTest(-1.111111, -2.11111, latEps, lonEps), ());
  TEST(ConvergenceTest(-90, -179.999999, latEps, lonEps), ());
  TEST(ConvergenceTest(-88.12313, 80.4532999999, latEps, lonEps), ());
}

UNIT_TEST(ZoomDecoding)
{
  TestSuccess("ge0://8wAAAAAAAA/Name", 0, 0, 19, "Name");
  TestSuccess("ge0://AwAAAAAAAA/Name", 0, 0, 4, "Name");
  TestSuccess("ge0://BwAAAAAAAA/Name", 0, 0, 4.25, "Name");
}

UNIT_TEST(LatLonDecoding)
{
  TestSuccess("ge0://Byqqqqqqqq/Name", 45, 0, 4.25, "Name");
  TestSuccess("ge0://B6qqqqqqqq/Name", 90, 0, 4.25, "Name");
  TestSuccess("ge0://BVVVVVVVVV/Name", -90, 179.999999, 4.25, "Name");
  TestSuccess("ge0://BP________/Name", -0.000001, -0.000001, 4.25, "Name");
  TestSuccess("ge0://Bzqqqqqqqq/Name", 45, 45, 4.25, "Name");
  TestSuccess("ge0://BaF6F6F6F6/Name", -20, 20, 4.25, "Name");
  TestSuccess("ge0://B4srhdHVVt/Name", 64.5234, 12.1234, 4.25, "Name");
  TestSuccess("ge0://B_________/Name", 90, 179.999999, 4.25, "Name");
  TestSuccess("ge0://Bqqqqqqqqq/Name", 90, -180, 4.25, "Name");
  TestSuccess("ge0://BAAAAAAAAA/Name", -90, -180, 4.25, "Name");
}

UNIT_TEST(NameDecoding)
{
  TestSuccess("ge0://AwAAAAAAAA/Super_Poi", 0, 0, 4, "Super Poi");
  TestSuccess("ge0://AwAAAAAAAA/Super%5FPoi", 0, 0, 4, "Super Poi");
  TestSuccess("ge0://AwAAAAAAAA/Super%5fPoi", 0, 0, 4, "Super Poi");
  TestSuccess("ge0://AwAAAAAAAA/Super Poi", 0, 0, 4, "Super_Poi");
  TestSuccess("ge0://AwAAAAAAAA/Super%20Poi", 0, 0, 4, "Super_Poi");
  TestSuccess("ge0://AwAAAAAAAA/Name%", 0, 0, 4, "Name");
  TestSuccess("ge0://AwAAAAAAAA/Name%2", 0, 0, 4, "Name");
  TestSuccess("ge0://AwAAAAAAAA/Hello%09World%0A", 0, 0, 4, "Hello\tWorld\n");
  TestSuccess("ge0://AwAAAAAAAA/Hello%%%%%%%%%", 0, 0, 4, "Hello");
  TestSuccess("ge0://AwAAAAAAAA/Hello%%%%%%%%%World", 0, 0, 4, "Hello");
  TestSuccess("ge0://AwAAAAAAAA/%d0%9c%d0%b8%d0%bd%d1%81%d0%ba_%d1%83%d0%bb._%d0%9b%d0%b5%d0%bd%d0%b8%d0%bd%d0%b0_9", 0, 0, 4, "Минск ул. Ленина 9");
  TestSuccess("ge0://AwAAAAAAAA/z%c3%bcrich_bahnhofstrasse", 0, 0, 4, "zürich bahnhofstrasse");
  TestSuccess("ge0://AwAAAAAAAA/%e5%8c%97%e4%ba%ac_or_B%c4%9bij%c4%abng%3F", 0, 0, 4, "北京 or Běijīng?");
  TestSuccess("ge0://AwAAAAAAAA/\xd1\x81\xd1\x82\xd1\x80\xd0\xbe\xd0\xba\xd0\xb0_\xd0\xb2_\xd1\x8e\xd1\x82\xd1\x84-8", 0, 0, 4, "строка в ютф-8");

  // name is valid, but too long
  //TestSuccess("ge0://AwAAAAAAAA/%d0%9a%d0%b0%d0%ba_%d0%b2%d1%8b_%d1%81%d1%87%d0%b8%d1%82%d0%b0%d0%b5%d1%82%d0%b5%2C_%d0%bd%d0%b0%d0%b4%d0%be_%d0%bb%d0%b8_%d0%bf%d0%b8%d1%81%d0%b0%d1%82%d1%8c_const_%d0%b4%d0%bb%d1%8f_%d0%bf%d0%b0%d1%80%d0%b0%d0%bc%d0%b5%d1%82%d1%80%d0%be%d0%b2%2C_%d0%ba%d0%be%d1%82%d0%be%d1%80%d1%8b%d0%b5_%d0%bf%d0%b5%d1%80%d0%b5%d0%b4%d0%b0%d1%8e%d1%82%d1%81%d1%8f_%d0%b2_%d1%84%d1%83%d0%bd%d0%ba%d1%86%d0%b8%d1%8e_%d0%bf%d0%be_%d0%b7%d0%bd%d0%b0%d1%87%d0%b5%d0%bd%d0%b8%d1%8e%3F",
  //            0, 0, 4, "Как вы считаете, надо ли писать const для параметров, которые передаются в функцию по значению?");
}

UNIT_TEST(LatLonFullAndClippedCoordinates)
{
  double maxLatDiffForCoordSize[10] = { 0 };
  double maxLonDiffForCoordSize[10] = { 0 };
  for (double lat = -90; lat <= 90; lat += 0.7)
  {
    for (double lon = -180; lon < 180; lon += 0.7)
    {
      char buf[20] = { 0 };
      MapsWithMe_GenShortShowMapUrl(lat, lon, 4, "", buf, ARRAY_SIZE(buf));
      for (int i = 9; i >= 1; --i)
      {
        string const str = string(buf).substr(7, i);
        size_t const coordSize = str.size();
        Ge0ParserForTest parser;
        double latTmp, lonTmp;
        parser.DecodeLatLon(str, latTmp, lonTmp);
        double const epsLat = GetLatEpsilon(coordSize);
        double const epsLon = GetLonEpsilon(coordSize);
        double const difLat = fabs(lat - latTmp);
        double const difLon = fabs(lon - lonTmp);
        TEST(difLat <= epsLat, (str, lat, latTmp, lon, lonTmp, difLat, epsLat));
        TEST(difLon <= epsLon, (str, lat, latTmp, lon, lonTmp, difLon, epsLon));
        maxLatDiffForCoordSize[coordSize] = max(maxLatDiffForCoordSize[coordSize], difLat);
        maxLonDiffForCoordSize[coordSize] = max(maxLonDiffForCoordSize[coordSize], difLon);
      }
    }
  }

  for (size_t coordSize = 1; coordSize <= 8; ++coordSize)
  {
    TEST(maxLatDiffForCoordSize[coordSize] > maxLatDiffForCoordSize[coordSize + 1], (coordSize));
    TEST(maxLonDiffForCoordSize[coordSize] > maxLonDiffForCoordSize[coordSize + 1], (coordSize));

    TEST(maxLatDiffForCoordSize[coordSize] <= GetLatEpsilon(coordSize), (coordSize));
    TEST(maxLonDiffForCoordSize[coordSize] <= GetLonEpsilon(coordSize), (coordSize));

    TEST(maxLatDiffForCoordSize[coordSize] > GetLatEpsilon(coordSize + 1), (coordSize));
    TEST(maxLonDiffForCoordSize[coordSize] > GetLonEpsilon(coordSize + 1), (coordSize));
  }
}

UNIT_TEST(ClippedName)
{
  TestSuccess("ge0://AwAAAAAAAA/Super%5fPoi", 0, 0, 4, "Super Poi");
  TestSuccess("ge0://AwAAAAAAAA/Super%5fPo" , 0, 0, 4, "Super Po");
  TestSuccess("ge0://AwAAAAAAAA/Super%5fP"  , 0, 0, 4, "Super P");
  TestSuccess("ge0://AwAAAAAAAA/Super%5f"   , 0, 0, 4, "Super ");
  TestSuccess("ge0://AwAAAAAAAA/Super%5"    , 0, 0, 4, "Super");
  TestSuccess("ge0://AwAAAAAAAA/Super%"     , 0, 0, 4, "Super");
  TestSuccess("ge0://AwAAAAAAAA/Super"      , 0, 0, 4, "Super");
  TestSuccess("ge0://AwAAAAAAAA/Supe"       , 0, 0, 4, "Supe");
  TestSuccess("ge0://AwAAAAAAAA/Sup"        , 0, 0, 4, "Sup");
  TestSuccess("ge0://AwAAAAAAAA/Su"         , 0, 0, 4, "Su");
  TestSuccess("ge0://AwAAAAAAAA/S"          , 0, 0, 4, "S");
  TestSuccess("ge0://AwAAAAAAAA/"           , 0, 0, 4, "");
  TestSuccess("ge0://AwAAAAAAAA"            , 0, 0, 4, "");
}
