#include "osm_time_range.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#define BOOST_TEST_MODULE OpeningHours
#include <boost/test/included/unit_test.hpp>
#pragma clang diagnostic pop


BOOST_AUTO_TEST_CASE(OpeningHours_StaticSet)
{
  {
    OSMTimeRange oh("09:00-19:00;Sa 09:00-18:00;Tu,Su,PH OFF"); // symbols case
    BOOST_CHECK(oh.IsValid());
  }
  {
    OSMTimeRange oh("09:00-19:00;Sa 09:00-18:00;Tu,Su,ph Off"); // symbols case
    BOOST_CHECK(oh.IsValid());
  }
  {
    OSMTimeRange oh("05:00 – 22:00"); // long dash instead minus
    BOOST_CHECK(oh.IsValid());
  }
  {
    OSMTimeRange oh("05:00 - 22:00"); // minus
    BOOST_CHECK(oh.IsValid());
  }
  {
    OSMTimeRange oh("09:00-20:00 open \"Bei schönem Wetter. Falls unklar kann angerufen werden\""); // charset
    BOOST_CHECK(oh.IsValid());
  }
  {
    OSMTimeRange oh("09:00-22:00; Tu off; dec 31 off; Jan 1 off"); // symbols case
    BOOST_CHECK(oh.IsValid());
  }
  {
    OSMTimeRange oh("9:00-22:00"); // leading zeros
    BOOST_CHECK(oh.IsValid());
  }
  {
    OSMTimeRange oh("09:00-9:30"); // leading zeros
    BOOST_CHECK(oh.IsValid());
  }
  {
    OSMTimeRange oh("Mo 08:00-11:00,14:00-17:00; Tu 08:00-11:00, 14:00-17:00; We 08:00-11:00; Th 08:00-11:00, 14:00-16:00; Fr 08:00-11:00");
    BOOST_CHECK(oh.IsValid());
  }

}

BOOST_AUTO_TEST_CASE( OpeningHours_FromFile )
{
//  BOOST_REQUIRE(false);
  std::ifstream datalist("opening.lst");
  BOOST_REQUIRE_MESSAGE(datalist.is_open(), "Can't open ./opening.lst: " << std::strerror(errno));

  std::string line;
  while (std::getline(datalist, line))
  {
    OSMTimeRange oh(line);
    BOOST_CHECK_MESSAGE(oh.IsValid(), "Can't parse: [" << line << "]");
  }
}