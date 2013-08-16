#include "../../testing/testing.hpp"

#include "../mwm_url.hpp"
#include "../../coding/uri.hpp"

#include "../../base/string_format.hpp"
#include "../../base/pseudo_random.hpp"
#include "../../base/logging.hpp"

using namespace url_scheme;

UNIT_TEST(MapApiSmoke)
{
  Uri uri("mapswithme://map?ll=38.970559,-9.419289&ignoreThisParam=Yes&z=17&n=Point%20Name");
  TEST(uri.IsValid(), ());

  ParsedMapApi api(uri);
  TEST(api.IsValid(), ());
  TEST_EQUAL(api.GetPoints().size(), 1, ());
  TEST_EQUAL(api.GetPoints()[0].m_lat, 38.970559, ());
  TEST_EQUAL(api.GetPoints()[0].m_lon, -9.419289, ());
  TEST_EQUAL(api.GetPoints()[0].m_name, "Point Name", ());
  TEST_EQUAL(api.GetPoints()[0].m_id, "", ());
  TEST_EQUAL(api.GetGlobalBackUrl(), "", ());
}

UNIT_TEST(MapApiInvalidUrl)
{
  TEST(!ParsedMapApi(Uri("competitors://map?ll=12.3,34.54")).IsValid(), ());
  TEST(!ParsedMapApi(Uri("mapswithme://ggg?ll=12.3,34.54")).IsValid(), ());
  TEST(!ParsedMapApi(Uri("mwm://")).IsValid(), ("No path"));
  TEST(!ParsedMapApi(Uri("mapswithme://map?")).IsValid(), ("No parameters"));
  TEST(!ParsedMapApi(Uri("mapswithme://map?ll=23.55")).IsValid(), ("No longtitude"));
  TEST(!ParsedMapApi(Uri("mapswithme://map?ll=1,2,3")).IsValid(), ("Too many values for ll"));
}

UNIT_TEST(MapApiLatLonLimits)
{
  TEST(!ParsedMapApi(Uri("mapswithme://map?ll=-91,10")).IsValid(), ("Invalid latitude"));
  TEST(!ParsedMapApi(Uri("mwm://map?ll=523.55,10")).IsValid(), ("Invalid latitude"));
  TEST(!ParsedMapApi(Uri("mapswithme://map?ll=23.55,450")).IsValid(), ("Invalid longtitude"));
  TEST(!ParsedMapApi(Uri("mapswithme://map?ll=23.55,-450")).IsValid(), ("Invalid longtitude"));
}

UNIT_TEST(MapApiPointNameBeforeLatLon)
{
  ParsedMapApi api(Uri("mapswithme://map?n=Name&ll=1,2"));
  TEST(api.IsValid(), ());
  TEST_EQUAL(api.GetPoints().size(), 1, ());
  TEST_EQUAL(api.GetPoints()[0].m_name, "", ());
}

UNIT_TEST(MapApiPointNameOverwritten)
{
  ParsedMapApi api(Uri("mapswithme://map?ll=1,2&n=A&N=B"));
  TEST(api.IsValid(), ());
  TEST_EQUAL(api.GetPoints().size(), 1, ());
  TEST_EQUAL(api.GetPoints()[0].m_name, "B", ());
}

UNIT_TEST(MapApiMultiplePoints)
{
  ParsedMapApi api(Uri("mwm://map?ll=1.1,1.2&n=A&LL=2.1,2.2&ll=-3.1,-3.2&n=C"));
  TEST(api.IsValid(), ());
  TEST_EQUAL(api.GetPoints().size(), 3, ());
  TEST_EQUAL(api.GetPoints()[0].m_lat, 1.1, ());
  TEST_EQUAL(api.GetPoints()[0].m_lon, 1.2, ());
  TEST_EQUAL(api.GetPoints()[0].m_name, "A", ());
  TEST_EQUAL(api.GetPoints()[1].m_name, "", ());
  TEST_EQUAL(api.GetPoints()[1].m_lat, 2.1, ());
  TEST_EQUAL(api.GetPoints()[1].m_lon, 2.2, ());
  TEST_EQUAL(api.GetPoints()[2].m_name, "C", ());
  TEST_EQUAL(api.GetPoints()[2].m_lat, -3.1, ());
  TEST_EQUAL(api.GetPoints()[2].m_lon, -3.2, ());
}

UNIT_TEST(MapApiInvalidPointLatLonButValidOtherParts)
{
  ParsedMapApi api(Uri("mapswithme://map?ll=1,1,1&n=A&ll=2,2&n=B&ll=3,3,3&n=C"));
  TEST(api.IsValid(), ());
  TEST_EQUAL(api.GetPoints().size(), 1, ());
  TEST_EQUAL(api.GetPoints()[0].m_lat, 2, ());
  TEST_EQUAL(api.GetPoints()[0].m_lon, 2, ());
  TEST_EQUAL(api.GetPoints()[0].m_name, "B", ());
}

UNIT_TEST(MapApiPointURLEncoded)
{
  ParsedMapApi api(Uri("mwm://map?ll=1,2&n=%D0%9C%D0%B8%D0%BD%D1%81%D0%BA&id=http%3A%2F%2Fmap%3Fll%3D1%2C2%26n%3Dtest"));
  TEST(api.IsValid(), ());
  TEST_EQUAL(api.GetPoints().size(), 1, ());
  TEST_EQUAL(api.GetPoints()[0].m_name, "\xd0\x9c\xd0\xb8\xd0\xbd\xd1\x81\xd0\xba", ());
  TEST_EQUAL(api.GetPoints()[0].m_id, "http://map?ll=1,2&n=test", ());
}

UNIT_TEST(GlobalBackUrl)
{
  {
    ParsedMapApi api(Uri("mwm://map?ll=1,2&n=PointName&backurl=someTestAppBackUrl"));
    TEST_EQUAL(api.GetGlobalBackUrl(), "someTestAppBackUrl://", ());
  }
  {
    ParsedMapApi api(Uri("mwm://map?ll=1,2&n=PointName&backurl=ge0://"));
    TEST_EQUAL(api.GetGlobalBackUrl(), "ge0://", ());
  }
  {
    ParsedMapApi api(Uri("mwm://map?ll=1,2&n=PointName&backurl=ge0%3A%2F%2F"));
    TEST_EQUAL(api.GetGlobalBackUrl(), "ge0://", ());
  }
  {
    ParsedMapApi api(Uri("mwm://map?ll=1,2&n=PointName&backurl=http://mapswithme.com"));
    TEST_EQUAL(api.GetGlobalBackUrl(), "http://mapswithme.com", ());
  }
  {
    ParsedMapApi api(Uri("mwm://map?ll=1,2&n=PointName&backUrl=someapp://%D0%9C%D0%BE%D0%B1%D0%B8%D0%BB%D1%8C%D0%BD%D1%8B%D0%B5%20%D0%9A%D0%B0%D1%80%D1%82%D1%8B"));
    TEST_EQUAL(api.GetGlobalBackUrl(), "someapp://\xd0\x9c\xd0\xbe\xd0\xb1\xd0\xb8\xd0\xbb\xd1\x8c\xd0\xbd\xd1\x8b\xd0\xb5 \xd0\x9a\xd0\xb0\xd1\x80\xd1\x82\xd1\x8b", ());
  }
  {
    ParsedMapApi api(Uri("mwm://map?ll=1,2&n=PointName"));
    TEST_EQUAL(api.GetGlobalBackUrl(), "", ());
  }
  {
    ParsedMapApi api(Uri("mwm://map?ll=1,2&n=PointName&backurl=%D0%BF%D1%80%D0%B8%D0%BB%D0%BE%D0%B6%D0%B5%D0%BD%D0%B8%D0%B5%3A%2F%2F%D0%BE%D1%82%D0%BA%D1%80%D0%BE%D0%B9%D0%A1%D1%81%D1%8B%D0%BB%D0%BA%D1%83"));
    TEST_EQUAL(api.GetGlobalBackUrl(), "приложение://откройСсылку", ());
  }
  {
    ParsedMapApi api(Uri("mwm://map?ll=1,2&n=PointName&backurl=%D0%BF%D1%80%D0%B8%D0%BB%D0%BE%D0%B6%D0%B5%D0%BD%D0%B8%D0%B5%3A%2F%2F%D0%BE%D1%82%D0%BA%D1%80%D0%BE%D0%B9%D0%A1%D1%81%D1%8B%D0%BB%D0%BA%D1%83"));
    TEST_EQUAL(api.GetGlobalBackUrl(), "приложение://откройСсылку", ());
  }
  {
    ParsedMapApi api(Uri("mwm://map?ll=1,2&n=PointName&backurl=%E6%88%91%E6%84%9Bmapswithme"));
    TEST_EQUAL(api.GetGlobalBackUrl(), "我愛mapswithme://", ());
  }
}

UNIT_TEST(VersionTest)
{
  {
    ParsedMapApi api(Uri("mwm://map?ll=1,2&v=1&n=PointName"));
    TEST_EQUAL(api.GetApiVersion(), 1, ());
  }
  {
    ParsedMapApi api(Uri("mwm://map?ll=1,2&v=kotik&n=PointName"));
    TEST_EQUAL(api.GetApiVersion(), 0, ());
  }
  {
    ParsedMapApi api(Uri("mwm://map?ll=1,2&v=APacanyVoobsheKotjata&n=PointName"));
    TEST_EQUAL(api.GetApiVersion(), 0, ());
  }
  {
    ParsedMapApi api(Uri("mwm://map?ll=1,2&n=PointName"));
    TEST_EQUAL(api.GetApiVersion(), 0, ());
  }
  {
    ParsedMapApi api(Uri("mwm://map?V=666&ll=1,2&n=PointName"));
    TEST_EQUAL(api.GetApiVersion(), 666, ());
  }
}

UNIT_TEST(AppNameTest)
{
  {
    ParsedMapApi api(Uri("mwm://map?ll=1,2&v=1&n=PointName&appname=Google"));
    TEST_EQUAL(api.GetAppTitle(), "Google", ());
  }
  {
    ParsedMapApi api(Uri("mwm://map?ll=1,2&v=1&n=PointName&AppName=%D0%AF%D0%BD%D0%B4%D0%B5%D0%BA%D1%81"));
    TEST_EQUAL(api.GetAppTitle(), "Яндекс", ());
  }
  {
    ParsedMapApi api(Uri("mwm://map?ll=1,2&v=1&n=PointName"));
    TEST_EQUAL(api.GetAppTitle(), "", ());
  }
}

UNIT_TEST(RectTest)
{
  {
    ParsedMapApi api(Uri("mwm://map?ll=0,0"));
    m2::RectD rect = api.GetLatLonRect();
    TEST_EQUAL(rect.maxX(), 0, ());
    TEST_EQUAL(rect.maxY(), 0, ());
    TEST_EQUAL(rect.minX(), 0, ());
    TEST_EQUAL(rect.minX(), 0, ());
  }
  {
    ParsedMapApi api(Uri("mwm://map?ll=0,0&ll=1,1&ll=2,2&ll=3,3&ll=4,4&ll=5,5&"));
    m2::RectD rect = api.GetLatLonRect();
    TEST_EQUAL(rect.maxX(), 5, ());
    TEST_EQUAL(rect.maxY(), 5, ());
    TEST_EQUAL(rect.minX(), 0, ());
    TEST_EQUAL(rect.minX(), 0, ());
  }
  {
    ParsedMapApi api(Uri("mwm://map?ll=-90,90&ll=90,-90"));
    m2::RectD rect = api.GetLatLonRect();
    TEST_EQUAL(rect.maxX(), 90, ());
    TEST_EQUAL(rect.maxY(), 90, ());
    TEST_EQUAL(rect.minX(), -90, ());
    TEST_EQUAL(rect.minX(), -90, ());
  }
  {
    ParsedMapApi api(Uri("mwm://map?ll=180,180&ll=0,0&ll=-180,-180"));
    m2::RectD rect = api.GetLatLonRect();
    TEST_EQUAL(rect.maxX(), 0, ());
    TEST_EQUAL(rect.maxY(), 0, ());
    TEST_EQUAL(rect.minX(), 0, ());
    TEST_EQUAL(rect.minX(), 0, ());
  }
  {
    ParsedMapApi api(Uri("mwm://"));
    m2::RectD rect = api.GetLatLonRect();
    TEST(!rect.IsValid(), ());
  }
}

namespace
{
string generatePartOfUrl(url_scheme::ApiPoint const & point)
{
  stringstream stream;
  stream << "&ll=" << strings::ToString(point.m_lat)  << "," << strings::ToString(point.m_lon)
         << "&n=" << point.m_name
         << "&id=" << point.m_id;
  return stream.str();
}

string randomString(size_t size, size_t seed)
{
  string result(size, '0');
  LCG32 random(seed);
  for (size_t i = 0; i < size; ++i)
    result[i] = 'a' + random.Generate() % 26;
  return result;
}

void generateRandomTest(size_t numberOfPoints, size_t stringLength)
{
  vector <url_scheme::ApiPoint> vect(numberOfPoints);
  for (size_t i = 0; i < numberOfPoints; ++i)
  {
    url_scheme::ApiPoint point;
    LCG32 random(i);
    point.m_lat = random.Generate() % 90;
    point.m_lat *= random.Generate() % 2 == 0 ? 1 : -1;
    point.m_lon = random.Generate() % 180;
    point.m_lon *= random.Generate() % 2 == 0 ? 1 : -1;
    point.m_name = randomString(stringLength, i);
    point.m_id = randomString(stringLength, i);
    vect[i] = point;
  }
  string result = "mapswithme://map?v=1";
  for (size_t i = 0; i < vect.size(); ++i)
    result += generatePartOfUrl(vect[i]);
  Uri uri(result);
  ParsedMapApi api(uri);
  vector <url_scheme::ApiPoint> const & points = api.GetPoints();
  TEST_EQUAL(points.size(), vect.size(), ());
  for (size_t i = 0; i < vect.size();++i)
  {
    TEST_EQUAL(points[i].m_lat, vect[i].m_lat, ());
    TEST_EQUAL(points[i].m_lon, vect[i].m_lon, ());
    TEST_EQUAL(points[i].m_name, vect[i].m_name, ());
    TEST_EQUAL(points[i].m_id, vect[i].m_id, ());
  }
  TEST_EQUAL(api.GetApiVersion(), 1, ());
}

}

UNIT_TEST(100FullEnteriesRandomTest)
{
  generateRandomTest(100, 10);
}

UNIT_TEST(StressTestRandomTest)
{
  generateRandomTest(10000, 100);
}

UNIT_TEST(MWMApiZoomLevelTest)
{
  m2::RectD const r1 = ParsedMapApi(Uri("mwm://map?ll=0,0")).GetLatLonRect();
  m2::RectD const r2 = ParsedMapApi(Uri("mwm://map?z=14.5&ll=0,0")).GetLatLonRect();
  TEST_NOT_EQUAL(r1, r2, ());
  m2::RectD const r3 = ParsedMapApi(Uri("mwm://map?ll=0,0&z=14")).GetLatLonRect();
  TEST_NOT_EQUAL(r2, r3, ());
  TEST_NOT_EQUAL(r1, r3, ());
  m2::RectD const rEqualToR3 = ParsedMapApi(Uri("mwm://map?ll=0,0&z=14.000")).GetLatLonRect();
  TEST_EQUAL(r3, rEqualToR3, ());
  m2::RectD const rEqualToR1 = ParsedMapApi(Uri("mwm://map?ll=0,0&z=-23.43")).GetLatLonRect();
  TEST_EQUAL(r1, rEqualToR1, ());
}

UNIT_TEST(MWMApiBalloonActionDefaultTest)
{
  {
    Uri uri("mapswithme://map?ll=38.970559,-9.419289&ignoreThisParam=Yes&z=17&n=Point%20Name");
    ParsedMapApi api(uri);
    TEST(!api.GoBackOnBalloonClick(), (""));
  }
  {
    Uri uri("mapswithme://map?ll=38.970559,-9.419289&ignoreThisParam=Yes&z=17&n=Point%20Name&balloonAction=false");
    ParsedMapApi api(uri);
    TEST(api.GoBackOnBalloonClick(), (""));
  }
  {
    Uri uri("mapswithme://map?ll=38.970559,-9.419289&ignoreThisParam=Yes&z=17&n=Point%20Name&balloonAction=true");
    ParsedMapApi api(uri);
    TEST(api.GoBackOnBalloonClick(), (""));
  }
  {
    Uri uri("mapswithme://map?ll=38.970559,-9.419289&ignoreThisParam=Yes&z=17&n=Point%20Name&balloonAction=");
    ParsedMapApi api(uri);
    TEST(api.GoBackOnBalloonClick(), (""));
  }
}
