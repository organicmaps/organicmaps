#include "testing/testing.hpp"

#include "map/mwm_url.hpp"

#include "geometry/mercator.hpp"

#include "coding/url.hpp"

#include "base/macros.hpp"

#include <random>
#include <string>

#include "defines.hpp"

namespace mwm_url_tests
{
using namespace std;
using namespace url_scheme;
using UrlType = ParsedMapApi::UrlType;

double const kEps = 1e-10;

UNIT_TEST(MapApiSmoke)
{
  string urlString =
      "mapswithme://"
      "map?ll=38.970559,-9.419289&ignoreThisParam=Yes&z=17&n=Point%20Name&s=black&backurl=https%3A%2F%2Forganicmaps."
      "app";
  TEST(url::Url(urlString).IsValid(), ());

  ParsedMapApi test(urlString);
  TEST_EQUAL(test.GetRequestType(), UrlType::Map, ());
  TEST_EQUAL(test.GetMapPoints().size(), 1, ());
  MapPoint const & p0 = test.GetMapPoints()[0];
  TEST_ALMOST_EQUAL_ABS(p0.m_lat, 38.970559, kEps, ());
  TEST_ALMOST_EQUAL_ABS(p0.m_lon, -9.419289, kEps, ());
  TEST_EQUAL(p0.m_name, "Point Name", ());
  TEST_EQUAL(p0.m_id, "", ());
  TEST_EQUAL(p0.m_style, "black", ());
  TEST_ALMOST_EQUAL_ABS(test.GetZoomLevel(), 17.0, kEps, ());
  TEST_EQUAL(test.GetGlobalBackUrl(), "https://organicmaps.app", ());
}

UNIT_TEST(RouteApiSmoke)
{
  string const urlString = "mapswithme://route?sll=1,1&saddr=name0&dll=2,2&daddr=name1&type=vehicle";
  TEST(url::Url(urlString).IsValid(), ());

  ParsedMapApi test(urlString);
  TEST_EQUAL(test.GetRequestType(), UrlType::Route, ());
  TEST_EQUAL(test.GetRoutePoints().size(), 2, ());
  RoutePoint const & p0 = test.GetRoutePoints()[0];
  RoutePoint const & p1 = test.GetRoutePoints()[1];
  TEST_EQUAL(p0.m_org, mercator::FromLatLon(1, 1), ());
  TEST_EQUAL(p0.m_name, "name0", ());
  TEST_EQUAL(p1.m_org, mercator::FromLatLon(2, 2), ());
  TEST_EQUAL(p1.m_name, "name1", ());
  TEST_EQUAL(test.GetRoutingType(), "vehicle", ());
}

UNIT_TEST(RouteApiV2MultipleStopsPreview)
{
  string const urlString =
      "om://v2/dir?origin=1,1&origin_name=Start&destination=3,3&destination_name=Finish"
      "&waypoints=2,2|2.5,2.5&waypoint_names=Coffee%20Stop|Bakery&mode=walk&unknown=value";
  TEST(url::Url(urlString).IsValid(), ());

  ParsedMapApi test(urlString);
  TEST_EQUAL(test.GetRequestType(), UrlType::Route, ());
  TEST_EQUAL(test.GetRoutePoints().size(), 4, ());
  RoutePoint const & p0 = test.GetRoutePoints()[0];
  RoutePoint const & p1 = test.GetRoutePoints()[1];
  RoutePoint const & p2 = test.GetRoutePoints()[2];
  RoutePoint const & p3 = test.GetRoutePoints()[3];
  TEST_EQUAL(p0.m_org, mercator::FromLatLon(1, 1), ());
  TEST_EQUAL(p0.m_name, "Start", ());
  TEST(!p0.m_isMyPosition, ());
  TEST_EQUAL(p1.m_org, mercator::FromLatLon(2, 2), ());
  TEST_EQUAL(p1.m_name, "Coffee Stop", ());
  TEST_EQUAL(p2.m_org, mercator::FromLatLon(2.5, 2.5), ());
  TEST_EQUAL(p2.m_name, "Bakery", ());
  TEST_EQUAL(p3.m_org, mercator::FromLatLon(3, 3), ());
  TEST_EQUAL(p3.m_name, "Finish", ());
  TEST_EQUAL(test.GetRoutingType(), "pedestrian", ());
  TEST_EQUAL(test.GetApiVersion(), 2, ());
  TEST(!test.ShouldOptimizeRoutePoints(), ());
  TEST(!test.ShouldStartRouteNavigation(), ());
}

UNIT_TEST(RouteApiV2NavigationUsesCurrentPositionByDefault)
{
  string const urlString =
      "om://v2/nav?destination=2,2&destination_name=Finish&mode=drive&optimize=true&origin_heading=90";
  TEST(url::Url(urlString).IsValid(), ());

  ParsedMapApi test(urlString);
  TEST_EQUAL(test.GetRequestType(), UrlType::Route, ());
  TEST_EQUAL(test.GetRoutePoints().size(), 2, ());
  TEST(test.GetRoutePoints()[0].m_isMyPosition, ());
  TEST_EQUAL(test.GetRoutePoints()[0].m_name, "", ());
  TEST_EQUAL(test.GetRoutePoints()[1].m_org, mercator::FromLatLon(2, 2), ());
  TEST_EQUAL(test.GetRoutePoints()[1].m_name, "Finish", ());
  TEST_EQUAL(test.GetRoutingType(), "vehicle", ());
  TEST(test.ShouldOptimizeRoutePoints(), ());
  TEST(test.ShouldStartRouteNavigation(), ());
  TEST_ALMOST_EQUAL_ABS(test.GetRouteStartDirection().x, 1.0, kEps, ());
  TEST_ALMOST_EQUAL_ABS(test.GetRouteStartDirection().y, 0.0, kEps, ());
}

UNIT_TEST(RouteApiV2ExplicitOriginNavigationBuildsPreview)
{
  ParsedMapApi test("om://v2/nav?origin=1,1&destination=2,2");
  TEST_EQUAL(test.GetRequestType(), UrlType::Route, ());
  TEST_EQUAL(test.GetRoutePoints().size(), 2, ());
  TEST(!test.GetRoutePoints()[0].m_isMyPosition, ());
  TEST_EQUAL(test.GetRoutePoints()[0].m_org, mercator::FromLatLon(1, 1), ());
  TEST(test.ShouldStartRouteNavigation(), ());
}

UNIT_TEST(RouteApiV2AllowsEmptyWaypoints)
{
  ParsedMapApi empty("om://v2/dir?origin=1,1&waypoints=&destination=2,2");
  TEST_EQUAL(empty.GetRequestType(), UrlType::Route, ());
  TEST_EQUAL(empty.GetRoutePoints().size(), 2, ());
  TEST_EQUAL(empty.GetRoutePoints()[0].m_org, mercator::FromLatLon(1, 1), ());
  TEST_EQUAL(empty.GetRoutePoints()[1].m_org, mercator::FromLatLon(2, 2), ());

  ParsedMapApi trailing("om://v2/dir?origin=1,1&waypoints=1.5,1.5|&destination=2,2");
  TEST_EQUAL(trailing.GetRequestType(), UrlType::Route, ());
  TEST_EQUAL(trailing.GetRoutePoints().size(), 3, ());
  TEST_EQUAL(trailing.GetRoutePoints()[1].m_org, mercator::FromLatLon(1.5, 1.5), ());

  ParsedMapApi gap(
      "om://v2/dir?origin=1,1&waypoints=1.5,1.5||2.5,2.5&waypoint_names=A|B|C"
      "&waypoint_callbacks=app%3A%2F%2F1|app%3A%2F%2F2|app%3A%2F%2F3&destination=3,3");
  TEST_EQUAL(gap.GetRequestType(), UrlType::Route, ());
  TEST_EQUAL(gap.GetRoutePoints().size(), 4, ());
  TEST_EQUAL(gap.GetRoutePoints()[1].m_name, "A", ());
  TEST_EQUAL(gap.GetRoutePoints()[1].m_callback, "app://1", ());
  TEST_EQUAL(gap.GetRoutePoints()[2].m_name, "C", ());
  TEST_EQUAL(gap.GetRoutePoints()[2].m_callback, "app://3", ());
}

UNIT_TEST(RouteApiV2CurrentPositionKeepsOriginFields)
{
  string const urlString = "om://v2/nav?destination=2,2&origin_name=Warehouse&origin_callback=app%3A%2F%2Forigin";

  ParsedMapApi test(urlString);
  TEST_EQUAL(test.GetRequestType(), UrlType::Route, ());
  TEST_EQUAL(test.GetRoutePoints().size(), 2, ());
  TEST(test.GetRoutePoints()[0].m_isMyPosition, ());
  TEST_EQUAL(test.GetRoutePoints()[0].m_name, "Warehouse", ());
  TEST_EQUAL(test.GetRoutePoints()[0].m_callback, "app://origin", ());
}

UNIT_TEST(RouteApiV2RejectsInvalidOriginHeading)
{
  ParsedMapApi test("om://v2/nav?destination=2,2&origin_heading=361");
  TEST_EQUAL(test.GetRequestType(), UrlType::Incorrect, ());
}

UNIT_TEST(RouteApiV2AcceptsHeadingBoundaries)
{
  ParsedMapApi north("om://v2/nav?destination=2,2&origin_heading=0");
  TEST_EQUAL(north.GetRequestType(), UrlType::Route, ());
  TEST_ALMOST_EQUAL_ABS(north.GetRouteStartDirection().x, 0.0, kEps, ());
  TEST_ALMOST_EQUAL_ABS(north.GetRouteStartDirection().y, 1.0, kEps, ());

  ParsedMapApi fullCircle("om://v2/nav?destination=2,2&origin_heading=360");
  TEST_EQUAL(fullCircle.GetRequestType(), UrlType::Route, ());
  TEST_ALMOST_EQUAL_ABS(fullCircle.GetRouteStartDirection().x, 0.0, kEps, ());
  TEST_ALMOST_EQUAL_ABS(fullCircle.GetRouteStartDirection().y, 1.0, kEps, ());
}

UNIT_TEST(RouteApiV2CallbacksAndBikeMode)
{
  string const urlString =
      "https://omaps.app/v2/dir?origin=1,1&origin_callback=app%3A%2F%2Forigin&destination=2,2"
      "&destination_callback=app%3A%2F%2Ffinish&waypoints=1.5,1.5&waypoint_callbacks=app%3A%2F%2Fstop"
      "&mode=bike&ref_name=DeliveryCo&callback=app%3A%2F%2Fback";

  ParsedMapApi test(urlString);
  TEST_EQUAL(test.GetRequestType(), UrlType::Route, ());
  TEST_EQUAL(test.GetRoutePoints().size(), 3, ());
  TEST_EQUAL(test.GetRoutePoints()[0].m_callback, "app://origin", ());
  TEST_EQUAL(test.GetRoutePoints()[1].m_callback, "app://stop", ());
  TEST_EQUAL(test.GetRoutePoints()[2].m_callback, "app://finish", ());
  TEST_EQUAL(test.GetRoutingType(), "bicycle", ());
  TEST_EQUAL(test.GetAppName(), "DeliveryCo", ());
  TEST_EQUAL(test.GetGlobalBackUrl(), "app://back", ());
}

UNIT_TEST(RouteApiV2PreservesEncodedPipesInWaypointCallbacks)
{
  string const urlString =
      "om://v2/dir?origin=1,1&destination=4,4&waypoints=2,2|3,3"
      "&waypoint_callbacks=app%3A%2F%2Fdone%3Fstate%3Da%7Cb|app%3A%2F%2Fnext";

  ParsedMapApi test(urlString);
  TEST_EQUAL(test.GetRequestType(), UrlType::Route, ());
  TEST_EQUAL(test.GetRoutePoints().size(), 4, ());
  TEST_EQUAL(test.GetRoutePoints()[1].m_callback, "app://done?state=a|b", ());
  TEST_EQUAL(test.GetRoutePoints()[2].m_callback, "app://next", ());
}

UNIT_TEST(RouteApiV2AcceptsGoogleMapsDirectionAliases)
{
  string const urlString =
      "om://v2/dir?api=1&origin=1,1&destination=2,2&travelmode=bicycling&dir_action=navigate"
      "&avoid=tolls%2Chighways";

  ParsedMapApi test(urlString);
  TEST_EQUAL(test.GetRequestType(), UrlType::Route, ());
  TEST_EQUAL(test.GetRoutingType(), "bicycle", ());
  TEST(test.ShouldStartRouteNavigation(), ());

  ParsedMapApi car("om://v2/dir?destination=2,2&mode=car");
  TEST_EQUAL(car.GetRequestType(), UrlType::Route, ());
  TEST_EQUAL(car.GetRoutingType(), "vehicle", ());

  ParsedMapApi driving("om://v2/dir?destination=2,2&mode=driving");
  TEST_EQUAL(driving.GetRequestType(), UrlType::Route, ());
  TEST_EQUAL(driving.GetRoutingType(), "vehicle", ());

  ParsedMapApi walking("om://v2/dir?destination=2,2&mode=walking");
  TEST_EQUAL(walking.GetRequestType(), UrlType::Route, ());
  TEST_EQUAL(walking.GetRoutingType(), "pedestrian", ());
}

UNIT_TEST(RouteApiV2HandlesMixedSignsAndAnyParameterOrder)
{
  string const urlString =
      "om://v2/nav?destination=-34.0522,18.5610&mode=walk&waypoints=-33.95,18.50&origin=-33.9249,18.4241";

  ParsedMapApi test(urlString);
  TEST_EQUAL(test.GetRequestType(), UrlType::Route, ());
  TEST(test.ShouldStartRouteNavigation(), ());
  TEST_EQUAL(test.GetRoutingType(), "pedestrian", ());
  TEST_EQUAL(test.GetRoutePoints().size(), 3, ());
  TEST_EQUAL(test.GetRoutePoints()[0].m_org, mercator::FromLatLon(-33.9249, 18.4241), ());
  TEST_EQUAL(test.GetRoutePoints()[1].m_org, mercator::FromLatLon(-33.95, 18.50), ());
  TEST_EQUAL(test.GetRoutePoints()[2].m_org, mercator::FromLatLon(-34.0522, 18.5610), ());
}

UNIT_TEST(RouteApiV2OptimizeFalsyValues)
{
  for (auto const & optimize : {"0", "false", ""})
  {
    ParsedMapApi test("om://v2/dir?destination=2,2&optimize=" + string(optimize));
    TEST_EQUAL(test.GetRequestType(), UrlType::Route, ());
    TEST(!test.ShouldOptimizeRoutePoints(), (optimize));
  }
}

UNIT_TEST(RouteApiLegacyAllowsCommonAppAndCenterParams)
{
  ParsedMapApi test(
      "om://route?appname=Foo&cll=1,2&sll=3,4&saddr=Start&dll=5,6&daddr=Finish&type=vehicle&backurl=app%3A%2F%2Fback");
  TEST_EQUAL(test.GetRequestType(), UrlType::Route, ());
  TEST_EQUAL(test.GetAppName(), "Foo", ());
  TEST_ALMOST_EQUAL_ABS(test.GetCenterLatLon().m_lat, 1.0, kEps, ());
  TEST_ALMOST_EQUAL_ABS(test.GetCenterLatLon().m_lon, 2.0, kEps, ());
  TEST_EQUAL(test.GetGlobalBackUrl(), "", ());
}

UNIT_TEST(SearchApiSmoke)
{
  string const urlString =
      "mapsme://search?query=Saint%20Hilarion&cll=35.3166654,33.2833322&locale=ru&map&appname=Organic%20Maps";
  TEST(url::Url(urlString).IsValid(), ());

  ParsedMapApi test(urlString);
  TEST_EQUAL(test.GetRequestType(), UrlType::Search, ());
  auto const & request = test.GetSearchRequest();
  ms::LatLon latlon = test.GetCenterLatLon();
  TEST_EQUAL(request.m_query, "Saint Hilarion", ());
  TEST_ALMOST_EQUAL_ABS(latlon.m_lat, 35.3166654, kEps, ());
  TEST_ALMOST_EQUAL_ABS(latlon.m_lon, 33.2833322, kEps, ());
  TEST_EQUAL(request.m_locale, "ru", ());
  TEST_EQUAL(test.GetAppName(), "Organic Maps", ());
  TEST(request.m_isSearchOnMap, ());
}

UNIT_TEST(SearchApiAdvanced)
{
  {
    // Ignore wrong cll=.
    ParsedMapApi test("om://search?query=aaa&cll=1,1,1");
    TEST_EQUAL(test.GetRequestType(), UrlType::Search, ());
    auto const & request = test.GetSearchRequest();
    ms::LatLon latlon = test.GetCenterLatLon();
    TEST_EQUAL(request.m_query, "aaa", ());
    TEST_EQUAL(request.m_locale, "", ());
    TEST(!request.m_isSearchOnMap, ());
    TEST_EQUAL(latlon.m_lat, ms::LatLon::kInvalid, ());
    TEST_EQUAL(latlon.m_lon, ms::LatLon::kInvalid, ());
  }

  {
    // Don't fail on unsupported parameters.
    ParsedMapApi test("om://search?query=aaa&ignoreThisParam=sure");
    TEST_EQUAL(test.GetRequestType(), UrlType::Search, ());
    auto const & request = test.GetSearchRequest();
    ms::LatLon latlon = test.GetCenterLatLon();
    TEST_EQUAL(request.m_query, "aaa", ());
    TEST_EQUAL(request.m_locale, "", ());
    TEST(!request.m_isSearchOnMap, ());
    TEST_EQUAL(latlon.m_lat, ms::LatLon::kInvalid, ());
    TEST_EQUAL(latlon.m_lon, ms::LatLon::kInvalid, ());
  }

  {
    // Query parameter position doesn't matter
    ParsedMapApi test("om://search?cll=1,1&locale=ru&query=aaa");
    TEST_EQUAL(test.GetRequestType(), UrlType::Search, ());
    auto const & request = test.GetSearchRequest();
    ms::LatLon latlon = test.GetCenterLatLon();
    TEST_EQUAL(request.m_query, "aaa", ());
    TEST_EQUAL(request.m_locale, "ru", ());
    TEST(!request.m_isSearchOnMap, ());
    TEST_ALMOST_EQUAL_ABS(latlon.m_lat, 1.0, kEps, ());
    TEST_ALMOST_EQUAL_ABS(latlon.m_lon, 1.0, kEps, ());
  }
}

UNIT_TEST(SearchApiInvalidUrl)
{
  ParsedMapApi test;
  TEST_EQUAL(test.SetUrlAndParse("mapsme://search?"), UrlType::Incorrect, ("Empty query string"));
  TEST_EQUAL(test.SetUrlAndParse("mapsme://search?query"), UrlType::Incorrect, ("Search query can't be empty"));
  TEST_EQUAL(test.SetUrlAndParse("mapsme://serch?cll=1,1&locale=ru&query=aaa"), UrlType::Incorrect,
             ("Incorrect url type"));
  TEST_EQUAL(test.SetUrlAndParse("mapsme://search?Query=fff"), UrlType::Incorrect, ("The parser is case sensitive"));
  TEST_EQUAL(test.SetUrlAndParse("incorrect://search?query=aaa"), UrlType::Incorrect, ("Wrong prefix"));
  TEST_EQUAL(test.SetUrlAndParse("http://search?query=aaa"), UrlType::Incorrect, ("Wrong prefix"));
}

UNIT_TEST(LeadApiSmoke)
{
  ParsedMapApi test;
  TEST_EQUAL(test.SetUrlAndParse("mapsme://lead?utm_source=a&utm_medium=b&utm_campaign=c&utm_content=d&utm_term=e"),
             UrlType::Incorrect, ("Lead API is not supported"));
}

UNIT_TEST(MapApiInvalidUrl)
{
  ParsedMapApi test;
  TEST_EQUAL(test.SetUrlAndParse("competitors://map?ll=12.3,34.54"), UrlType::Incorrect, ());
  TEST_EQUAL(test.SetUrlAndParse("mapswithme://ggg?ll=12.3,34.54"), UrlType::Incorrect, ());
  TEST_EQUAL(test.SetUrlAndParse("mwm://"), UrlType::Incorrect, ("No parameters"));
  TEST_EQUAL(test.SetUrlAndParse("mapswithme://map?"), UrlType::Incorrect, ("No longtitude"));
  TEST_EQUAL(test.SetUrlAndParse("mapswithme://map?ll=1,2,3"), UrlType::Incorrect, ("Too many values for ll"));
  TEST_EQUAL(test.SetUrlAndParse("mapswithme://fffff://map?ll=1,2"), UrlType::Incorrect, ());
  TEST_EQUAL(test.SetUrlAndParse("mapsme://map?LL=1,1"), UrlType::Incorrect, ("The parser is case sensitive"));
}

UNIT_TEST(RouteApiInvalidUrl)
{
  ParsedMapApi test;
  TEST_EQUAL(test.SetUrlAndParse("mapswithme://route?sll=1,1&saddr=name0&dll=2,2&daddr=name2"), UrlType::Incorrect,
             ("Route type doesn't exist"));
  TEST_EQUAL(test.SetUrlAndParse("mapswithme://route?sll=1,1&saddr=name0"), UrlType::Incorrect,
             ("Destination doesn't exist"));
  TEST_EQUAL(test.SetUrlAndParse("mapswithme://route?sll=1,1&dll=2,2&type=vehicle"), UrlType::Incorrect,
             ("Source or destination name doesn't exist"));
  TEST_EQUAL(test.SetUrlAndParse("mapswithme://route?saddr=name0&daddr=name1&type=vehicle"), UrlType::Incorrect, ());
  TEST_EQUAL(test.SetUrlAndParse("mapswithme://route?sll=1,1&sll=2.2&type=vehicle"), UrlType::Incorrect, ());
  TEST_EQUAL(test.SetUrlAndParse("mapswithme://route?sll=1,1&dll=2.2&type=666"), UrlType::Incorrect, ());
  TEST_EQUAL(test.SetUrlAndParse("mapswithme://route?sll=1,1&saddr=name0&sll=2,2&saddr=name1&type=vehicle"),
             UrlType::Incorrect, ());
  TEST_EQUAL(test.SetUrlAndParse("mapswithme://route?sll=1,1&type=vehicle"), UrlType::Incorrect, ());
  TEST_EQUAL(test.SetUrlAndParse(
                 "mapswithme://route?sll=1,1&saddr=name0&sll=2,2&saddr=name1&sll=1,1&saddr=name0&type=vehicle"),
             UrlType::Incorrect, ());
  TEST_EQUAL(test.SetUrlAndParse("mapswithme://route?type=vehicle"), UrlType::Incorrect, ());
  TEST_EQUAL(test.SetUrlAndParse("mapswithme://rout?sll=1,1&saddr=name0&dll=2,2&daddr=name1&type=vehicle"),
             UrlType::Incorrect, ());
  TEST_EQUAL(test.SetUrlAndParse("om://route?ll=1,1&type=vehicle"), UrlType::Incorrect,
             ("Legacy route parser does not accept v2 points"));
  TEST_EQUAL(test.SetUrlAndParse("om://v2/dir?origin=1,1&mode=drive"), UrlType::Incorrect,
             ("V2 route requires destination"));
  TEST_EQUAL(test.SetUrlAndParse("om://v2/nav?destination=2,2&mode=spaceship"), UrlType::Incorrect,
             ("V2 route rejects unknown mode values"));
  TEST_EQUAL(test.SetUrlAndParse("om://route?ll=1,1&saddr=name0&dll=2,2&daddr=name1&type=vehicle"), UrlType::Incorrect,
             ("V2 points cannot be mixed into the legacy route parser"));
}

UNIT_TEST(MapApiLatLonLimits)
{
  ParsedMapApi test;
  TEST_EQUAL(test.SetUrlAndParse("mapswithme://map?ll=-91,10"), UrlType::Incorrect, ("Invalid latitude"));
  TEST_EQUAL(test.SetUrlAndParse("mwm://map?ll=523.55,10"), UrlType::Incorrect, ("Invalid latitude"));
  TEST_EQUAL(test.SetUrlAndParse("mapswithme://map?ll=23.55,450"), UrlType::Incorrect, ("Invalid longtitude"));
  TEST_EQUAL(test.SetUrlAndParse("mapswithme://map?ll=23.55,-450"), UrlType::Incorrect, ("Invalid longtitude"));
}

UNIT_TEST(MapApiPointNameBeforeLatLon)
{
  ParsedMapApi test("mapswithme://map?n=Name&ll=1,2");
  TEST_EQUAL(test.GetRequestType(), UrlType::Incorrect, ());
}

UNIT_TEST(MapApiPointNameOverwritten)
{
  {
    ParsedMapApi api("mapswithme://map?ll=1,2&n=A&N=B");
    TEST_EQUAL(api.GetRequestType(), UrlType::Map, ());
    TEST_EQUAL(api.GetMapPoints().size(), 1, ());
    TEST_EQUAL(api.GetMapPoints()[0].m_name, "A", ());
  }

  {
    ParsedMapApi api("mapswithme://map?ll=1,2&n=A&n=B");
    TEST_EQUAL(api.GetRequestType(), UrlType::Map, ());
    TEST_EQUAL(api.GetMapPoints().size(), 1, ());
    TEST_EQUAL(api.GetMapPoints()[0].m_name, "B", ());
  }
}

UNIT_TEST(MapApiMultiplePoints)
{
  ParsedMapApi api("mwm://map?ll=1.1,1.2&n=A&ll=2.1,2.2&ll=-3.1,-3.2&n=C");
  TEST_EQUAL(api.GetRequestType(), UrlType::Map, ());
  TEST_EQUAL(api.GetMapPoints().size(), 3, ());
  MapPoint const & p0 = api.GetMapPoints()[0];
  TEST_ALMOST_EQUAL_ABS(p0.m_lat, 1.1, kEps, ());
  TEST_ALMOST_EQUAL_ABS(p0.m_lon, 1.2, kEps, ());
  TEST_EQUAL(p0.m_name, "A", ());
  MapPoint const & p1 = api.GetMapPoints()[1];
  TEST_ALMOST_EQUAL_ABS(p1.m_lat, 2.1, kEps, ());
  TEST_ALMOST_EQUAL_ABS(p1.m_lon, 2.2, kEps, ());
  TEST_EQUAL(p1.m_name, "", ());
  MapPoint const & p2 = api.GetMapPoints()[2];
  TEST_ALMOST_EQUAL_ABS(p2.m_lat, -3.1, kEps, ());
  TEST_ALMOST_EQUAL_ABS(p2.m_lon, -3.2, kEps, ());
  TEST_EQUAL(p2.m_name, "C", ());
}

UNIT_TEST(MapApiInvalidPointLatLonButValidOtherParts)
{
  ParsedMapApi api("mapswithme://map?ll=1,1,1&n=A&ll=2,2&n=B&ll=3,3,3&n=C");
  TEST_EQUAL(api.GetRequestType(), UrlType::Incorrect, ());
}

UNIT_TEST(MapApiPointURLEncoded)
{
  ParsedMapApi api("mwm://map?ll=1,2&n=%D0%9C%D0%B8%D0%BD%D1%81%D0%BA&id=http%3A%2F%2Fmap%3Fll%3D1%2C2%26n%3Dtest");
  TEST_EQUAL(api.GetRequestType(), UrlType::Map, ());
  TEST_EQUAL(api.GetMapPoints().size(), 1, ());
  MapPoint const & p0 = api.GetMapPoints()[0];
  TEST_EQUAL(p0.m_name, "\xd0\x9c\xd0\xb8\xd0\xbd\xd1\x81\xd0\xba", ());
  TEST_EQUAL(p0.m_id, "http://map?ll=1,2&n=test", ());
}

UNIT_TEST(MapApiUrl)
{
  {
    ParsedMapApi api("https://www.google.com/maps?q=55.751809,37.6130029");
    TEST_EQUAL(api.GetRequestType(), UrlType::Map, ());
    TEST_EQUAL(api.GetMapPoints().size(), 1, ());
    MapPoint const & p0 = api.GetMapPoints()[0];
    TEST_ALMOST_EQUAL_ABS(p0.m_lat, 55.751809, kEps, ());
    TEST_ALMOST_EQUAL_ABS(p0.m_lon, 37.6130029, kEps, ());
  }
  {
    ParsedMapApi api("https://www.openstreetmap.org/#map=16/33.89041/35.50664");
    TEST_EQUAL(api.GetRequestType(), UrlType::Map, ());
    TEST_EQUAL(api.GetMapPoints().size(), 1, ());
    MapPoint const & p0 = api.GetMapPoints()[0];
    TEST_ALMOST_EQUAL_ABS(p0.m_lat, 33.89041, kEps, ());
    TEST_ALMOST_EQUAL_ABS(p0.m_lon, 35.50664, kEps, ());
  }
  {
    ParsedMapApi api("ftp://www.google.com/maps?q=55.751809,37.6130029");
    TEST_EQUAL(api.GetRequestType(), UrlType::Incorrect, ());
  }
}

UNIT_TEST(MapApiGe0)
{
  {
    ParsedMapApi api("om://o4B4pYZsRs");
    TEST_EQUAL(api.GetRequestType(), UrlType::Map, ());
    TEST_EQUAL(api.GetMapPoints().size(), 1, ());
    MapPoint const & p0 = api.GetMapPoints()[0];
    TEST_ALMOST_EQUAL_ABS(p0.m_lat, 47.3859, 1e-4, ());
    TEST_ALMOST_EQUAL_ABS(p0.m_lon, 8.5766, 1e-4, ());
  }

  {
    ParsedMapApi api("om://o4B4pYZsRs/Zoo_Zürich");
    TEST_EQUAL(api.GetRequestType(), UrlType::Map, ());
    TEST_EQUAL(api.GetMapPoints().size(), 1, ());
    MapPoint const & p0 = api.GetMapPoints()[0];
    TEST_ALMOST_EQUAL_ABS(p0.m_lat, 47.3859, 1e-4, ());
    TEST_ALMOST_EQUAL_ABS(p0.m_lon, 8.5766, 1e-4, ());
    TEST_EQUAL(p0.m_name, "Zoo Zürich", ());
  }
  {
    ParsedMapApi api("http://omaps.app/o4B4pYZsRs/Zoo_Zürich");
    TEST_EQUAL(api.GetRequestType(), UrlType::Map, ());
  }
  {
    ParsedMapApi api("https://omaps.app/o4B4pYZsRs/Zoo_Zürich");
    TEST_EQUAL(api.GetRequestType(), UrlType::Map, ());
  }
  {
    ParsedMapApi api("ge0://o4B4pYZsRs/Zoo_Zürich");
    TEST_EQUAL(api.GetRequestType(), UrlType::Map, ());
  }
  {
    ParsedMapApi api("http://ge0.me/o4B4pYZsRs/Zoo_Zürich");
    TEST_EQUAL(api.GetRequestType(), UrlType::Map, ());
  }
  {
    ParsedMapApi api("https://ge0.me/o4B4pYZsRs/Zoo_Zürich");
    TEST_EQUAL(api.GetRequestType(), UrlType::Map, ());
  }
  {
    ParsedMapApi api("mapsme://o4B4pYZsRs/Zoo_Zürich");
    TEST_EQUAL(api.GetRequestType(), UrlType::Incorrect, ());
  }
  {
    ParsedMapApi api("mwm://o4B4pYZsRs/Zoo_Zürich");
    TEST_EQUAL(api.GetRequestType(), UrlType::Incorrect, ());
  }
  {
    ParsedMapApi api("mapswithme://o4B4pYZsRs/Zoo_Zürich");
    TEST_EQUAL(api.GetRequestType(), UrlType::Incorrect, ());
  }
}

UNIT_TEST(MapApiGeoScheme)
{
  {
    ParsedMapApi api("geo:0,0?q=35.341714,33.32231 (Custom%20Title)");
    TEST_EQUAL(api.GetRequestType(), UrlType::Map, ());
    TEST_EQUAL(api.GetMapPoints().size(), 1, ());
    MapPoint const & p0 = api.GetMapPoints()[0];
    TEST_ALMOST_EQUAL_ABS(p0.m_lat, 35.341714, kEps, ());
    TEST_ALMOST_EQUAL_ABS(p0.m_lon, 33.32231, kEps, ());
    TEST_EQUAL(p0.m_name, "Custom Title", ());
  }
  {
    ParsedMapApi api("geo:0,0?q=");
    TEST_EQUAL(api.GetRequestType(), UrlType::Map, ());
    TEST_EQUAL(api.GetMapPoints().size(), 1, ());
    MapPoint const & p0 = api.GetMapPoints()[0];
    TEST_ALMOST_EQUAL_ABS(p0.m_lat, 0.0, kEps, ());
    TEST_ALMOST_EQUAL_ABS(p0.m_lon, 0.0, kEps, ());
  }
  {
    ParsedMapApi api("geo:0,0");
    TEST_EQUAL(api.GetRequestType(), UrlType::Map, ());
    TEST_EQUAL(api.GetMapPoints().size(), 1, ());
    MapPoint const & p0 = api.GetMapPoints()[0];
    TEST_ALMOST_EQUAL_ABS(p0.m_lat, 0.0, kEps, ());
    TEST_ALMOST_EQUAL_ABS(p0.m_lon, 0.0, kEps, ());
  }
}

UNIT_TEST(SearchApiGeoScheme)
{
  {
    ParsedMapApi api("geo:0,0?q=Kyrenia%20Castle");
    TEST_EQUAL(api.GetRequestType(), UrlType::Search, ());
    auto const & request = api.GetSearchRequest();
    ms::LatLon latlon = api.GetCenterLatLon();
    TEST(!latlon.IsValid(), ());
    TEST_EQUAL(request.m_query, "Kyrenia Castle", ());
  }
  {
    ParsedMapApi api("geo:35.3381607,33.3290564?q=Kyrenia%20Castle");
    TEST_EQUAL(api.GetRequestType(), UrlType::Search, ());
    auto const & request = api.GetSearchRequest();
    ms::LatLon latlon = api.GetCenterLatLon();
    TEST_EQUAL(request.m_query, "Kyrenia Castle", ());
    TEST_ALMOST_EQUAL_ABS(latlon.m_lat, 35.3381607, kEps, ());
    TEST_ALMOST_EQUAL_ABS(latlon.m_lon, 33.3290564, kEps, ());
  }
  {
    ParsedMapApi api("geo:0,0?q=123+Main+St,+Seattle,+WA+98101");
    TEST_EQUAL(api.GetRequestType(), UrlType::Search, ());
    auto const & request = api.GetSearchRequest();
    ms::LatLon latlon = api.GetCenterLatLon();
    TEST(!latlon.IsValid(), ());
    TEST_EQUAL(request.m_query, "123 Main St, Seattle, WA 98101", ());
  }
}

UNIT_TEST(CrosshairApi)
{
  {
    ParsedMapApi api("om://crosshair?cll=47.3813,8.5889&appname=Google%20Maps");
    TEST_EQUAL(api.GetRequestType(), UrlType::Crosshair, ());
    ms::LatLon latlon = api.GetCenterLatLon();
    TEST_ALMOST_EQUAL_ABS(latlon.m_lat, 47.3813, kEps, ());
    TEST_ALMOST_EQUAL_ABS(latlon.m_lon, 8.5889, kEps, ());
    TEST_EQUAL(api.GetAppName(), "Google Maps", ());
  }
  {
    ParsedMapApi api("https://omaps.app/crosshair?cll=47.3813,8.5889&appname=Google%20Maps");
    TEST_EQUAL(api.GetRequestType(), UrlType::Crosshair, ());
    ms::LatLon latlon = api.GetCenterLatLon();
    TEST_ALMOST_EQUAL_ABS(latlon.m_lat, 47.3813, kEps, ());
    TEST_ALMOST_EQUAL_ABS(latlon.m_lon, 8.5889, kEps, ());
    TEST_EQUAL(api.GetAppName(), "Google Maps", ());
  }
}

UNIT_TEST(GlobalBackUrl)
{
  {
    ParsedMapApi api("mwm://map?ll=1,2&n=PointName&backurl=someTestAppBackUrl");
    TEST_EQUAL(api.GetGlobalBackUrl(), "someTestAppBackUrl://", ());
  }
  {
    ParsedMapApi api("mwm://map?ll=1,2&n=PointName&backurl=ge0://");
    TEST_EQUAL(api.GetGlobalBackUrl(), "ge0://", ());
  }
  {
    ParsedMapApi api("om://map?ll=1,2&n=PointName&backurl=om://");
    TEST_EQUAL(api.GetGlobalBackUrl(), "om://", ());
  }
  {
    ParsedMapApi api("mwm://map?ll=1,2&n=PointName&backurl=ge0%3A%2F%2F");
    TEST_EQUAL(api.GetGlobalBackUrl(), "ge0://", ());
  }
  {
    ParsedMapApi api("mwm://map?ll=1,2&n=PointName&backurl=http://mapswithme.com");
    TEST_EQUAL(api.GetGlobalBackUrl(), "http://mapswithme.com", ());
  }
  {
    ParsedMapApi api(
        "mwm://map?ll=1,2&n=PointName&backurl=someapp://"
        "%D0%9C%D0%BE%D0%B1%D0%B8%D0%BB%D1%8C%D0%BD%D1%8B%D0%B5%20%D0%9A%D0%B0%D1%80%D1%82%D1%8B");
    TEST_EQUAL(api.GetGlobalBackUrl(),
               "someapp://\xd0\x9c\xd0\xbe\xd0\xb1\xd0\xb8\xd0\xbb\xd1\x8c\xd0\xbd\xd1\x8b\xd0\xb5 "
               "\xd0\x9a\xd0\xb0\xd1\x80\xd1\x82\xd1\x8b",
               ());
  }
  {
    ParsedMapApi api("mwm://map?ll=1,2&n=PointName");
    TEST_EQUAL(api.GetGlobalBackUrl(), "", ());
  }
  {
    ParsedMapApi api(
        "mwm://"
        "map?ll=1,2&n=PointName&backurl=%D0%BF%D1%80%D0%B8%D0%BB%D0%BE%D0%B6%D0%B5%D0%BD%D0%B8%D0%B5%3A%2F%2F%D0%BE%D1%"
        "82%D0%BA%D1%80%D0%BE%D0%B9%D0%A1%D1%81%D1%8B%D0%BB%D0%BA%D1%83");
    TEST_EQUAL(api.GetGlobalBackUrl(), "приложение://откройСсылку", ());
  }
  {
    ParsedMapApi api(
        "mwm://"
        "map?ll=1,2&n=PointName&backurl=%D0%BF%D1%80%D0%B8%D0%BB%D0%BE%D0%B6%D0%B5%D0%BD%D0%B8%D0%B5%3A%2F%2F%D0%BE%D1%"
        "82%D0%BA%D1%80%D0%BE%D0%B9%D0%A1%D1%81%D1%8B%D0%BB%D0%BA%D1%83");
    TEST_EQUAL(api.GetGlobalBackUrl(), "приложение://откройСсылку", ());
  }
  {
    ParsedMapApi api("mwm://map?ll=1,2&n=PointName&backurl=%E6%88%91%E6%84%9Bmapswithme");
    TEST_EQUAL(api.GetGlobalBackUrl(), "我愛mapswithme://", ());
  }
}

UNIT_TEST(VersionTest)
{
  {
    ParsedMapApi api("mwm://map?ll=1,2&v=1&n=PointName");
    TEST_EQUAL(api.GetApiVersion(), 1, ());
  }
  {
    ParsedMapApi api("mwm://map?ll=1,2&v=kotik&n=PointName");
    TEST_EQUAL(api.GetApiVersion(), 0, ());
  }
  {
    ParsedMapApi api("mwm://map?ll=1,2&v=APacanyVoobsheKotjata&n=PointName");
    TEST_EQUAL(api.GetApiVersion(), 0, ());
  }
  {
    ParsedMapApi api("mwm://map?ll=1,2&n=PointName");
    TEST_EQUAL(api.GetApiVersion(), 0, ());
  }
  {
    ParsedMapApi api("mwm://map?V=666&ll=1,2&n=PointName");
    TEST_EQUAL(api.GetApiVersion(), 0, ());
  }
  {
    ParsedMapApi api("mwm://map?v=666&ll=1,2&n=PointName");
    TEST_EQUAL(api.GetApiVersion(), 666, ());
  }
}

UNIT_TEST(AppNameTest)
{
  {
    ParsedMapApi api("mwm://map?ll=1,2&v=1&n=PointName&appname=Google");
    TEST_EQUAL(api.GetAppName(), "Google", ());
  }
  {
    ParsedMapApi api("mwm://map?ll=1,2&v=1&n=PointName&appname=%D0%AF%D0%BD%D0%B4%D0%B5%D0%BA%D1%81");
    TEST_EQUAL(api.GetAppName(), "Яндекс", ());
  }
  {
    ParsedMapApi api("mwm://map?ll=1,2&v=1&n=PointName");
    TEST_EQUAL(api.GetAppName(), "", ());
  }
}

UNIT_TEST(OAuth2Test)
{
  {
    ParsedMapApi api("om://oauth2/osm/callback?code=THE_MEGA_CODE");
    TEST_EQUAL(api.GetRequestType(), UrlType::OAuth2, ());
    TEST_EQUAL(api.GetOAuth2Code(), "THE_MEGA_CODE", ());
  }
  {
    ParsedMapApi api("om://oauth2/google/callback?code=THE_MEGA_CODE");
    TEST_EQUAL(api.GetRequestType(), UrlType::Incorrect, ());
  }
  {
    ParsedMapApi api("om://oauth2/osm/callback?code=");
    TEST_EQUAL(api.GetRequestType(), UrlType::Incorrect, ());
  }
}

namespace
{
string generatePartOfUrl(url_scheme::MapPoint const & point)
{
  stringstream stream;
  stream << "&ll=" << std::to_string(point.m_lat) << "," << std::to_string(point.m_lon) << "&n=" << point.m_name
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
  vector<url_scheme::MapPoint> vect(numberOfPoints);
  for (uint32_t i = 0; i < numberOfPoints; ++i)
  {
    url_scheme::MapPoint point;
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

  ParsedMapApi api(result);
  TEST_EQUAL(api.GetRequestType(), UrlType::Map, ());
  TEST_EQUAL(api.GetMapPoints().size(), vect.size(), ());
  for (size_t i = 0; i < vect.size(); ++i)
  {
    MapPoint const & p = api.GetMapPoints()[i];
    TEST_EQUAL(p.m_name, vect[i].m_name, ());
    TEST_EQUAL(p.m_id, vect[i].m_id, ());
    TEST(AlmostEqualULPs(p.m_lat, vect[i].m_lat), ());
    TEST(AlmostEqualULPs(p.m_lon, vect[i].m_lon), ());
  }
}

}  // namespace

UNIT_TEST(100FullEnteriesRandomTest)
{
  generateRandomTest(100, 10);
}

UNIT_TEST(StressTestRandomTest)
{
  generateRandomTest(10000, 100);
}

}  // namespace mwm_url_tests
