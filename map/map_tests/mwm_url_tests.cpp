#include "../../testing/testing.hpp"

#include "../mwm_url.hpp"
#include "../../coding/uri.hpp"


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
  TEST_EQUAL(api.GetPoints()[0].m_title, "Point Name", ());
  TEST_EQUAL(api.GetPoints()[0].m_url, "", ());
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
  TEST_EQUAL(api.GetPoints()[0].m_title, "", ());
}

UNIT_TEST(MapApiPointNameOverwritten)
{
  ParsedMapApi api(Uri("mapswithme://map?ll=1,2&n=A&n=B"));
  TEST(api.IsValid(), ());
  TEST_EQUAL(api.GetPoints().size(), 1, ());
  TEST_EQUAL(api.GetPoints()[0].m_title, "B", ());
}

UNIT_TEST(MapApiMultiplePoints)
{
  ParsedMapApi api(Uri("mwm://map?ll=1.1,1.2&n=A&ll=2.1,2.2&ll=-3.1,-3.2&n=C"));
  TEST(api.IsValid(), ());
  TEST_EQUAL(api.GetPoints().size(), 3, ());
  TEST_EQUAL(api.GetPoints()[0].m_lat, 1.1, ());
  TEST_EQUAL(api.GetPoints()[0].m_lon, 1.2, ());
  TEST_EQUAL(api.GetPoints()[0].m_title, "A", ());
  TEST_EQUAL(api.GetPoints()[1].m_title, "", ());
  TEST_EQUAL(api.GetPoints()[1].m_lat, 2.1, ());
  TEST_EQUAL(api.GetPoints()[1].m_lon, 2.2, ());
  TEST_EQUAL(api.GetPoints()[2].m_title, "C", ());
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
  TEST_EQUAL(api.GetPoints()[0].m_title, "B", ());
}

UNIT_TEST(MapApiPointURLEncoded)
{
  ParsedMapApi api(Uri("mwm://map?ll=1,2&n=%D0%9C%D0%B8%D0%BD%D1%81%D0%BA&u=http%3A%2F%2Fmap%3Fll%3D1%2C2%26n%3Dtest"));
  TEST(api.IsValid(), ());
  TEST_EQUAL(api.GetPoints().size(), 1, ());
  TEST_EQUAL(api.GetPoints()[0].m_title, "\xd0\x9c\xd0\xb8\xd0\xbd\xd1\x81\xd0\xba", ());
  TEST_EQUAL(api.GetPoints()[0].m_url, "http://map?ll=1,2&n=test", ());
}
