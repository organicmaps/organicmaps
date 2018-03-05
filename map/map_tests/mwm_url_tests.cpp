#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/mwm_url.hpp"

#include "drape_frontend/visual_params.hpp"

#include "platform/settings.hpp"

#include "coding/uri.hpp"

#include "base/string_format.hpp"

#include "std/random.hpp"

using namespace url_scheme;

namespace
{
static FrameworkParams const kFrameworkParams(false /* m_enableLocalAds */, false /* m_enableDiffs */);

void ToMercatoToLatLon(double & lat, double & lon)
{
  lon = MercatorBounds::XToLon(MercatorBounds::LonToX(lon));
  lat = MercatorBounds::YToLat(MercatorBounds::LatToY(lat));
}

UserMark::Type const type = UserMark::Type::API;

class ApiTest
{
public:
  ApiTest(string const & uriString)
    : m_fm(kFrameworkParams)
  {
    m_m = &m_fm.GetBookmarkManager();
    m_api.SetBookmarkManager(m_m);

    auto const res = m_api.SetUriAndParse(uriString);
    if (res != ParsedMapApi::ParsingResult::Incorrect)
    {
      if (!m_api.GetViewportRect(m_viewportRect))
        m_viewportRect = df::GetWorldRect();
    }
  }

  bool IsValid() const { return m_api.IsValid(); }
  m2::RectD GetViewport() const { return m_viewportRect; }

  string const & GetAppTitle() const { return m_api.GetAppTitle(); }
  bool GoBackOnBalloonClick() const { return m_api.GoBackOnBalloonClick(); }

  size_t GetPointCount() const
  {
    return m_m->GetUserMarkIds(type).size();
  }

  vector<RoutePoint> GetRoutePoints() const { return m_api.GetRoutePoints(); }
  url_scheme::SearchRequest const & GetSearchRequest() const { return m_api.GetSearchRequest(); }
  string const & GetGlobalBackUrl() const { return m_api.GetGlobalBackUrl(); }
  int GetApiVersion() const { return m_api.GetApiVersion(); }

  bool TestLatLon(int index, double lat, double lon) const
  {
    ms::LatLon const ll = GetMark(index)->GetLatLon();
    return my::AlmostEqualULPs(ll.lat, lat) && my::AlmostEqualULPs(ll.lon, lon);
  }

  bool TestRoutePoint(int index, double lat, double lon, string const & name)
  {
    RoutePoint const pt = GetRoutePoints()[index];
    return pt.m_org == MercatorBounds::FromLatLon(lat, lon) && pt.m_name == name;
  }

  bool TestName(int index, string const & name) const
  {
    return GetMark(index)->GetName() == name;
  }

  bool TestID(int index, string const & id) const
  {
    return GetMark(index)->GetApiID() == id;
  }

  bool TestRouteType(string const & type) const { return m_api.GetRoutingType() == type; }
private:
  ApiMarkPoint const * GetMark(int index) const
  {
    auto const & markIds = m_m->GetUserMarkIds(type);
    TEST_LESS(index, static_cast<int>(markIds.size()), ());
    auto it = markIds.begin();
    std::advance(it, index);
    return m_m->GetMark<ApiMarkPoint>(*it);
  }

private:
  Framework m_fm;
  ParsedMapApi m_api;
  m2::RectD m_viewportRect;
  BookmarkManager * m_m;
};

bool IsValid(Framework & fm, string const & uriString)
{
  ParsedMapApi api;
  api.SetBookmarkManager(&fm.GetBookmarkManager());
  api.SetUriAndParse(uriString);
  fm.GetBookmarkManager().GetEditSession().ClearGroup(UserMark::Type::API);

  return api.IsValid();
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

UNIT_TEST(RouteApiSmoke)
{
  string const uriString =
      "mapswithme://route?sll=1,1&saddr=name0&dll=2,2&daddr=name1&type=vehicle";
  TEST(Uri(uriString).IsValid(), ());

  ApiTest test(uriString);
  TEST(test.IsValid(), ());
  TEST(test.TestRoutePoint(0, 1, 1, "name0"), ());
  TEST(test.TestRoutePoint(1, 2, 2, "name1"), ());
  TEST(test.TestRouteType("vehicle"), ());
}

UNIT_TEST(SearchApiSmoke)
{
  string const uriString = "mapsme://search?query=fff&cll=1,1&locale=ru&map";
  TEST(Uri(uriString).IsValid(), ());

  ApiTest test(uriString);
  TEST(test.IsValid(), ());

  auto const & request = test.GetSearchRequest();
  TEST_EQUAL(request.m_query, "fff", ());
  TEST_EQUAL(request.m_centerLat, 1, ());
  TEST_EQUAL(request.m_centerLon, 1, ());
  TEST_EQUAL(request.m_locale, "ru", ());
  TEST(request.m_isSearchOnMap, ());
}

UNIT_TEST(SearchApiInvalidUrl)
{
  Framework f(kFrameworkParams);
  TEST(!IsValid(f, "mapsme://search?"), ("The search query parameter is necessary"));
  TEST(!IsValid(f, "mapsme://search?query"), ("Search query can't be empty"));
  TEST(IsValid(f, "mapsme://search?query=aaa&cll=1,1,1"), ("If it's wrong lat lon format then just ignore it"));
  TEST(IsValid(f, "mapsme://search?query=aaa&ignoreThisParam=sure"), ("We shouldn't fail search request if there are some unsupported parameters"));
  TEST(IsValid(f, "mapsme://search?cll=1,1&locale=ru&query=aaa"), ("Query parameter position doesn't matter"));
  TEST(!IsValid(f, "mapsme://search?Query=fff"), ("The parser is case sensitive"));
}

UNIT_TEST(LeadApiSmoke)
{
  string const uriString = "mapsme://lead?utm_source=a&utm_medium=b&utm_campaign=c&utm_content=d&utm_term=e";
  TEST(Uri(uriString).IsValid(), ());
  ApiTest test(uriString);
  TEST(test.IsValid(), ());

  auto checkEqual = [](string const & key, string const & value)
  {
    string result;
    TEST(marketing::Settings::Get(key, result), ());
    TEST_EQUAL(result, value, ());
  };

  checkEqual("utm_source", "a");
  checkEqual("utm_medium", "b");
  checkEqual("utm_campaign", "c");
  checkEqual("utm_content", "d");
  checkEqual("utm_term", "e");
}

UNIT_TEST(LeadApiInvalid)
{
  Framework f(kFrameworkParams);
  TEST(!IsValid(f, "mapsme://lead?"), ("From, type and name parameters are necessary"));
  TEST(!IsValid(f, "mapsme://lead?utm_source&utm_medium&utm_campaign"), ("Parameters can't be empty"));
  TEST(IsValid(f, "mapsme://lead?utm_source=a&utm_medium=b&utm_campaign=c"), ("These parameters are enough"));
  TEST(IsValid(f, "mapsme://lead?utm_source=a&utm_medium=b&utm_campaign=c&smh=smh"), ("If there is an excess parameter just ignore it"));
  TEST(!IsValid(f, "mapsme://lead?utm_source=a&UTM_MEDIUM=b&utm_campaign=c&smh=smh"), ("The parser is case sensitive"));
}

UNIT_TEST(MapApiInvalidUrl)
{
  Framework fm(kFrameworkParams);
  TEST(!IsValid(fm, "competitors://map?ll=12.3,34.54"), ());
  TEST(!IsValid(fm, "mapswithme://ggg?ll=12.3,34.54"), ());
  TEST(!IsValid(fm, "mwm://"), ("No parameters"));
  TEST(!IsValid(fm, "mapswithme://map?"), ("No longtitude"));
  TEST(!IsValid(fm, "mapswithme://map?ll=1,2,3"), ("Too many values for ll"));
  TEST(!IsValid(fm, "mapswithme://fffff://map?ll=1,2"), ());
  TEST(!IsValid(fm, "mapsme://map?LL=1,1"), ("The parser is case sensitive"));
}

UNIT_TEST(RouteApiInvalidUrl)
{
  Framework f(kFrameworkParams);
  TEST(!IsValid(f, "mapswithme://route?sll=1,1&saddr=name0&dll=2,2&daddr=name2"),
       ("Route type doesn't exist"));
  TEST(!IsValid(f, "mapswithme://route?sll=1,1&saddr=name0"), ("Destination doesn't exist"));
  TEST(!IsValid(f, "mapswithme://route?sll=1,1&dll=2,2&type=vehicle"),
       ("Source or destination name doesn't exist"));
  TEST(!IsValid(f, "mapswithme://route?saddr=name0&daddr=name1&type=vehicle"), ());
  TEST(!IsValid(f, "mapswithme://route?sll=1,1&sll=2.2&type=vehicle"), ());
  TEST(!IsValid(f, "mapswithme://route?sll=1,1&dll=2.2&type=666"), ());
  TEST(!IsValid(f, "mapswithme://route?sll=1,1&saddr=name0&sll=2,2&saddr=name1&type=vehicle"), ());
  TEST(!IsValid(f, "mapswithme://route?sll=1,1&type=vehicle"), ());
  TEST(!IsValid(f,
                "mapswithme://"
                "route?sll=1,1&saddr=name0&sll=2,2&saddr=name1&sll=1,1&saddr=name0&type=vehicle"),
       ());
  TEST(!IsValid(f, "mapswithme://route?type=vehicle"), ());
}

UNIT_TEST(MapApiLatLonLimits)
{
  Framework fm(kFrameworkParams);
  TEST(!IsValid(fm, "mapswithme://map?ll=-91,10"), ("Invalid latitude"));
  TEST(!IsValid(fm, "mwm://map?ll=523.55,10"), ("Invalid latitude"));
  TEST(!IsValid(fm, "mapswithme://map?ll=23.55,450"), ("Invalid longtitude"));
  TEST(!IsValid(fm, "mapswithme://map?ll=23.55,-450"), ("Invalid longtitude"));
}

UNIT_TEST(MapApiPointNameBeforeLatLon)
{
  ApiTest test("mapswithme://map?n=Name&ll=1,2");
  TEST(!test.IsValid(), ());
  TEST_EQUAL(test.GetPointCount(), 0, ());
}

UNIT_TEST(MapApiPointNameOverwritten)
{
  {
    ApiTest api("mapswithme://map?ll=1,2&n=A&N=B");
    TEST(api.IsValid(), ());
    TEST_EQUAL(api.GetPointCount(), 1, ());
    TEST(api.TestName(0, "A"), ());
  }

  {
    ApiTest api("mapswithme://map?ll=1,2&n=A&n=B");
    TEST(api.IsValid(), ());
    TEST(api.TestName(0, "B"), ());
  }
}

UNIT_TEST(MapApiMultiplePoints)
{
  ApiTest api("mwm://map?ll=1.1,1.2&n=A&ll=2.1,2.2&ll=-3.1,-3.2&n=C");
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
  TEST(!api.IsValid(), ());
  TEST_EQUAL(api.GetPointCount(), 0, ());
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
    ApiTest api("mwm://map?ll=1,2&n=PointName&backurl=someapp://%D0%9C%D0%BE%D0%B1%D0%B8%D0%BB%D1%8C%D0%BD%D1%8B%D0%B5%20%D0%9A%D0%B0%D1%80%D1%82%D1%8B");
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
    TEST_EQUAL(api.GetApiVersion(), 0, ());
  }
  {
    ApiTest api("mwm://map?v=666&ll=1,2&n=PointName");
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
    ApiTest api("mwm://map?ll=1,2&v=1&n=PointName&appname=%D0%AF%D0%BD%D0%B4%D0%B5%D0%BA%D1%81");
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

string randomString(size_t size, uint32_t seed)
{
  string result(size, '0');
  mt19937 rng(seed);
  for (size_t i = 0; i < size; ++i)
    result[i] = 'a' + rng() % 26;
  return result;
}

void generateRandomTest(uint32_t numberOfPoints, size_t stringLength)
{
  vector <url_scheme::ApiPoint> vect(numberOfPoints);
  for (uint32_t i = 0; i < numberOfPoints; ++i)
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
  for (size_t i = 0; i < vect.size(); ++i)
  {
    /// Mercator defined not on all range of lat\lon values.
    /// Some part of lat\lon is clamp on convertation
    /// By this we convert  source data lat\lon to mercator and then into lat\lon
    /// to emulate core convertions
    double lat = vect[i].m_lat;
    double lon = vect[i].m_lon;
    ToMercatoToLatLon(lat, lon);
    int const ix = static_cast<int>(vect.size() - i) - 1;
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
    ApiTest api("mapswithme://map?ll=38.970559,-9.419289&ignoreThisParam=Yes&z=17&n=Point%20Name&balloonaction=false");
    TEST(api.GoBackOnBalloonClick(), (""));
  }
  {
    ApiTest api("mapswithme://map?ll=38.970559,-9.419289&ignoreThisParam=Yes&z=17&n=Point%20Name&balloonaction=true");
    TEST(api.GoBackOnBalloonClick(), (""));
  }
  {
    ApiTest api("mapswithme://map?ll=38.970559,-9.419289&ignoreThisParam=Yes&z=17&n=Point%20Name&balloonaction=");
    TEST(api.GoBackOnBalloonClick(), (""));
  }
}
