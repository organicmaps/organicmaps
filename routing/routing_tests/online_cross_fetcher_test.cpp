#include "testing/testing.hpp"

#include "routing/online_absent_fetcher.hpp"
#include "routing/online_cross_fetcher.hpp"

#include "geometry/point2d.hpp"

#include "std/algorithm.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

using namespace routing;

namespace
{
UNIT_TEST(UrlGeneratorTest)
{
  TEST_EQUAL(GenerateOnlineRequest("http://mapsme.test.ru:10012", ms::LatLon(55.690105, 37.726536),
                                   ms::LatLon(44.527843, 39.902344)),
             "http://mapsme.test.ru:10012/mapsme?loc=55.6901,37.7265&loc=44.5278,39.9023",
             ("Url parsed"));
}

UNIT_TEST(GoodResponseParse)
{
  vector<m2::PointD> outPoints;
  TEST(ParseResponse(R"({"used_mwms":[[39.522671,94.009263], [37.4809, 67.7244]]})", outPoints),
       ("Valid response can't be parsed"));
  TEST_EQUAL(outPoints.size(), 2, ());
  TEST(
      find(outPoints.begin(), outPoints.end(), m2::PointD(39.522671, 94.009263)) != outPoints.end(),
      ("Can't find point in server's response."));
  TEST(find(outPoints.begin(), outPoints.end(), m2::PointD(37.4809, 67.7244)) != outPoints.end(),
       ("Can't find point in server's response."));
}

UNIT_TEST(BadResponseParse)
{
  vector<m2::PointD> outPoints;
  TEST(!ParseResponse("{\"used_mwms\":[]}", outPoints), ("Inval response should not be parsed"));
  TEST_EQUAL(outPoints.size(), 0, ("Found mwm points in invalid request"));
}

UNIT_TEST(GarbadgeInputToResponseParser)
{
  vector<m2::PointD> outPoints;
  TEST(!ParseResponse("{\"usedsfblbvlshbvldshbvfvmdfknvksvbksvk", outPoints),
       ("Malformed response should not be processed."));
  TEST_EQUAL(outPoints.size(), 0, ("Found mwm points in invalid request"));
}

UNIT_TEST(OnlineAbsentFetcherSingleMwmTest)
{
  OnlineAbsentCountriesFetcher fetcher([](m2::PointD const & p){return "A";}, [](string const &){return false;});
  fetcher.GenerateRequest(Checkpoints({1, 1}, {2, 2}));
  vector<string> countries;
  fetcher.GetAbsentCountries(countries);
  TEST(countries.empty(), ());
}

}
