#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/mwm_url.hpp"

#include "coding/uri.hpp"

#include "base/string_format.hpp"

#include "std/random.hpp"

using namespace url_scheme;

namespace
{
  void ToMercatoToLatLon(double & lat, double & lon)
  {
    lon = MercatorBounds::XToLon(MercatorBounds::LonToX(lon));
    lat = MercatorBounds::YToLat(MercatorBounds::LatToY(lat));
  }

  const UserMarkContainer::Type type = UserMarkContainer::API_MARK;
  class ApiTest
  {
  public:
    ApiTest(string const & uriString)
    {
      m_m = &m_fm.GetBookmarkManager();
      m_c = &m_m->UserMarksGetController(type);
      m_api.SetController(m_c);
      m_api.SetUriAndParse(uriString);
    }

    bool IsValid() const { return m_api.IsValid(); }
    m2::RectD GetViewport()
    {
      m2::RectD rect;
      m_api.GetViewportRect(rect);
      return rect;
    }
    string const & GetAppTitle() { return m_api.GetAppTitle(); }
    bool GoBackOnBalloonClick() { return m_api.GoBackOnBalloonClick(); }
    int GetPointCount() { return m_c->GetUserMarkCount(); }
    string const & GetGlobalBackUrl() { return m_api.GetGlobalBackUrl(); }
    int GetApiVersion() { return m_api.GetApiVersion(); }
    bool TestLatLon(int index, double lat, double lon) const
    {
      double tLat, tLon;
      GetMark(index)->GetLatLon(tLat, tLon);
      return my::AlmostEqualULPs(tLat, lat) && my::AlmostEqualULPs(tLon, lon);
    }

    bool TestName(int index, string const & name)
    {
      return GetMark(index)->GetName() == name;
    }

    bool TestID(int index, string const & id)
    {
      return GetMark(index)->GetID() == id;
    }

  private:
    ApiMarkPoint const * GetMark(int index) const
    {
      TEST_LESS(index, m_c->GetUserMarkCount(), ());
      return static_cast<ApiMarkPoint const *>(m_c->GetUserMark(index));
    }

  private:
    Framework m_fm;
    ParsedMapApi m_api;
    UserMarkContainer::Controller * m_c;
    BookmarkManager * m_m;
  };

  bool IsValid(Framework & fm, string const & uriStrig)
  {
    ParsedMapApi api;
    UserMarkContainer::Type type = UserMarkContainer::API_MARK;
    api.SetController(&fm.GetBookmarkManager().UserMarksGetController(type));
    api.SetUriAndParse(uriStrig);
    bool res = api.IsValid();
    fm.GetBookmarkManager().UserMarksClear(type);
    return res;
  }
}

UNIT_TEST(MapApiSmoke)
{
  string uriString = "mapswithme://map?ll=38.970559,-9.419289&ignoreThisParam=Yes&z=17&n=Point%20Name";
  TEST(Uri(uriString).IsValid(), ());

  ApiTest test(uriString);

  TEST(test.IsValid(), ());
  TEST_EQUAL(test.GetPointCount(), 1, ());
  TEST(test.TestLatLon(0, 38.970559, -9.419289), ());
  TEST(test.TestName(0, "Point Name"), ());
  TEST(test.TestID(0, ""), ());
  TEST_EQUAL(test.GetGlobalBackUrl(), "", ());
}

UNIT_TEST(MapApiInvalidUrl)
{
  Framework fm;
  TEST(!IsValid(fm, "competitors://map?ll=12.3,34.54"), ());
  TEST(!IsValid(fm, "mapswithme://ggg?ll=12.3,34.54"), ());
  TEST(!IsValid(fm, "mwm://"), ("No parameters"));
  TEST(!IsValid(fm, "mapswithme://map?"), ("No longtitude"));
  TEST(!IsValid(fm, "mapswithme://map?ll=1,2,3"), ("Too many values for ll"));
}

UNIT_TEST(MapApiLatLonLimits)
{
  Framework fm;
  TEST(!IsValid(fm, "mapswithme://map?ll=-91,10"), ("Invalid latitude"));
  TEST(!IsValid(fm, "mwm://map?ll=523.55,10"), ("Invalid latitude"));
  TEST(!IsValid(fm, "mapswithme://map?ll=23.55,450"), ("Invalid longtitude"));
  TEST(!IsValid(fm, "mapswithme://map?ll=23.55,-450"), ("Invalid longtitude"));
}

UNIT_TEST(MapApiPointNameBeforeLatLon)
{
  ApiTest test("mapswithme://map?n=Name&ll=1,2");
  TEST(test.IsValid(), ());
  TEST_EQUAL(test.GetPointCount(), 1, ());
  TEST(test.TestName(0, ""), ());
}

UNIT_TEST(MapApiPointNameOverwritten)
{
  ApiTest api("mapswithme://map?ll=1,2&n=A&N=B");
  TEST(api.IsValid(), ());
  TEST_EQUAL(api.GetPointCount(), 1, ());
  TEST(api.TestName(0, "B"), ());
}

UNIT_TEST(MapApiMultiplePoints)
{
  ApiTest api("mwm://map?ll=1.1,1.2&n=A&LL=2.1,2.2&ll=-3.1,-3.2&n=C");
  TEST(api.IsValid(), ());
  TEST_EQUAL(api.GetPointCount(), 3, ());
  TEST(api.TestLatLon(2, 1.1, 1.2), ());
  TEST(api.TestName(2, "A"), ());
  TEST(api.TestLatLon(1, 2.1, 2.2), ());
  TEST(api.TestName(1, ""), ());
  TEST(api.TestLatLon(0, -3.1, -3.2), ());
  TEST(api.TestName(0, "C"), ());
}

UNIT_TEST(MapApiInvalidPointLatLonButValidOtherParts)
{
  ApiTest api("mapswithme://map?ll=1,1,1&n=A&ll=2,2&n=B&ll=3,3,3&n=C");
  TEST(api.IsValid(), ());
  TEST_EQUAL(api.GetPointCount(), 1, ());
  TEST(api.TestLatLon(0, 2, 2), ());
  TEST(api.TestName(0, "B"), ());
}

UNIT_TEST(MapApiPointURLEncoded)
{
  ApiTest api("mwm://map?ll=1,2&n=%D0%9C%D0%B8%D0%BD%D1%81%D0%BA&id=http%3A%2F%2Fmap%3Fll%3D1%2C2%26n%3Dtest");
  TEST(api.IsValid(), ());
  TEST_EQUAL(api.GetPointCount(), 1, ());
  TEST(api.TestName(0, "\xd0\x9c\xd0\xb8\xd0\xbd\xd1\x81\xd0\xba"), ());
  TEST(api.TestID(0, "http://map?ll=1,2&n=test"), ());
}

UNIT_TEST(GlobalBackUrl)
{
  {
    ApiTest api("mwm://map?ll=1,2&n=PointName&backurl=someTestAppBackUrl");
    TEST_EQUAL(api.GetGlobalBackUrl(), "someTestAppBackUrl://", ());
  }
  {
    ApiTest api("mwm://map?ll=1,2&n=PointName&backurl=ge0://");
    TEST_EQUAL(api.GetGlobalBackUrl(), "ge0://", ());
  }
  {
    ApiTest api("mwm://map?ll=1,2&n=PointName&backurl=ge0%3A%2F%2F");
    TEST_EQUAL(api.GetGlobalBackUrl(), "ge0://", ());
  }
  {
    ApiTest api("mwm://map?ll=1,2&n=PointName&backurl=http://mapswithme.com");
    TEST_EQUAL(api.GetGlobalBackUrl(), "http://mapswithme.com", ());
  }
  {
    ApiTest api("mwm://map?ll=1,2&n=PointName&backUrl=someapp://%D0%9C%D0%BE%D0%B1%D0%B8%D0%BB%D1%8C%D0%BD%D1%8B%D0%B5%20%D0%9A%D0%B0%D1%80%D1%82%D1%8B");
    TEST_EQUAL(api.GetGlobalBackUrl(), "someapp://\xd0\x9c\xd0\xbe\xd0\xb1\xd0\xb8\xd0\xbb\xd1\x8c\xd0\xbd\xd1\x8b\xd0\xb5 \xd0\x9a\xd0\xb0\xd1\x80\xd1\x82\xd1\x8b", ());
  }
  {
    ApiTest api("mwm://map?ll=1,2&n=PointName");
    TEST_EQUAL(api.GetGlobalBackUrl(), "", ());
  }
  {
    ApiTest api("mwm://map?ll=1,2&n=PointName&backurl=%D0%BF%D1%80%D0%B8%D0%BB%D0%BE%D0%B6%D0%B5%D0%BD%D0%B8%D0%B5%3A%2F%2F%D0%BE%D1%82%D0%BA%D1%80%D0%BE%D0%B9%D0%A1%D1%81%D1%8B%D0%BB%D0%BA%D1%83");
    TEST_EQUAL(api.GetGlobalBackUrl(), "приложение://откройСсылку", ());
  }
  {
    ApiTest api("mwm://map?ll=1,2&n=PointName&backurl=%D0%BF%D1%80%D0%B8%D0%BB%D0%BE%D0%B6%D0%B5%D0%BD%D0%B8%D0%B5%3A%2F%2F%D0%BE%D1%82%D0%BA%D1%80%D0%BE%D0%B9%D0%A1%D1%81%D1%8B%D0%BB%D0%BA%D1%83");
    TEST_EQUAL(api.GetGlobalBackUrl(), "приложение://откройСсылку", ());
  }
  {
    ApiTest api("mwm://map?ll=1,2&n=PointName&backurl=%E6%88%91%E6%84%9Bmapswithme");
    TEST_EQUAL(api.GetGlobalBackUrl(), "我愛mapswithme://", ());
  }
}

UNIT_TEST(VersionTest)
{
  {
    ApiTest api("mwm://map?ll=1,2&v=1&n=PointName");
    TEST_EQUAL(api.GetApiVersion(), 1, ());
  }
  {
    ApiTest api("mwm://map?ll=1,2&v=kotik&n=PointName");
    TEST_EQUAL(api.GetApiVersion(), 0, ());
  }
  {
    ApiTest api("mwm://map?ll=1,2&v=APacanyVoobsheKotjata&n=PointName");
    TEST_EQUAL(api.GetApiVersion(), 0, ());
  }
  {
    ApiTest api("mwm://map?ll=1,2&n=PointName");
    TEST_EQUAL(api.GetApiVersion(), 0, ());
  }
  {
    ApiTest api("mwm://map?V=666&ll=1,2&n=PointName");
    TEST_EQUAL(api.GetApiVersion(), 666, ());
  }
}

UNIT_TEST(AppNameTest)
{
  {
    ApiTest api("mwm://map?ll=1,2&v=1&n=PointName&appname=Google");
    TEST_EQUAL(api.GetAppTitle(), "Google", ());
  }
  {
    ApiTest api("mwm://map?ll=1,2&v=1&n=PointName&AppName=%D0%AF%D0%BD%D0%B4%D0%B5%D0%BA%D1%81");
    TEST_EQUAL(api.GetAppTitle(), "Яндекс", ());
  }
  {
    ApiTest api("mwm://map?ll=1,2&v=1&n=PointName");
    TEST_EQUAL(api.GetAppTitle(), "", ());
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
  mt19937 rng(seed);
  for (size_t i = 0; i < size; ++i)
    result[i] = 'a' + rng() % 26;
  return result;
}

void generateRandomTest(size_t numberOfPoints, size_t stringLength)
{
  vector <url_scheme::ApiPoint> vect(numberOfPoints);
  for (size_t i = 0; i < numberOfPoints; ++i)
  {
    url_scheme::ApiPoint point;
    mt19937 rng(i);
    point.m_lat = rng() % 90;
    point.m_lat *= rng() % 2 == 0 ? 1 : -1;
    point.m_lon = rng() % 180;
    point.m_lon *= rng() % 2 == 0 ? 1 : -1;
    point.m_name = randomString(stringLength, i);
    point.m_id = randomString(stringLength, i);
    vect[i] = point;
  }
  string result = "mapswithme://map?v=1";
  for (size_t i = 0; i < vect.size(); ++i)
    result += generatePartOfUrl(vect[i]);

  ApiTest api(result);
  TEST_EQUAL(api.GetPointCount(), vect.size(), ());
  for (size_t i = 0; i < vect.size();++i)
  {
    /// Mercator defined not on all range of lat\lon values.
    /// Some part of lat\lon is clamp on convertation
    /// By this we convert  source data lat\lon to mercator and then into lat\lon
    /// to emulate core convertions
    double lat = vect[i].m_lat;
    double lon = vect[i].m_lon;
    ToMercatoToLatLon(lat, lon);
    size_t const ix = vect.size() - i - 1;
    TEST(api.TestLatLon(ix, lat, lon), ());
    TEST(api.TestName(ix, vect[i].m_name), ());
    TEST(api.TestID(ix, vect[i].m_id), ());
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

UNIT_TEST(MWMApiBalloonActionDefaultTest)
{
  {
    ApiTest api("mapswithme://map?ll=38.970559,-9.419289&ignoreThisParam=Yes&z=17&n=Point%20Name");
    TEST(!api.GoBackOnBalloonClick(), (""));
  }
  {
    ApiTest api("mapswithme://map?ll=38.970559,-9.419289&ignoreThisParam=Yes&z=17&n=Point%20Name&balloonAction=false");
    TEST(api.GoBackOnBalloonClick(), (""));
  }
  {
    ApiTest api("mapswithme://map?ll=38.970559,-9.419289&ignoreThisParam=Yes&z=17&n=Point%20Name&balloonAction=true");
    TEST(api.GoBackOnBalloonClick(), (""));
  }
  {
    ApiTest api("mapswithme://map?ll=38.970559,-9.419289&ignoreThisParam=Yes&z=17&n=Point%20Name&balloonAction=");
    TEST(api.GoBackOnBalloonClick(), (""));
  }
}
