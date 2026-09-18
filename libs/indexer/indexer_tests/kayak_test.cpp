#include "testing/testing.hpp"

#include "indexer/kayak.hpp"

#include "private.h"

using namespace osm;

UNIT_TEST(GetKayakHotelURL)
{
  TEST_EQUAL(GetKayakHotelURL("TR", 2651619, "Elexus Hotel Resort & Spa & Casino", 7163, 1696321580, 1696407980),
             "https://www.kayak.com.tr/in?a=" KAYAK_AFFILIATE_ID
             "&url="
             "/hotels/Elexus-Hotel-Resort--Spa--Casino,-c7163-h2651619-details/2023-10-03/2023-10-04/1adults",
             ());
  TEST_EQUAL(GetKayakHotelURL("XX", 93048, "Acapulco Resort Convention Spa", 7163, 1696321580, 1696407980),
             "https://www.kayak.com/in?a=" KAYAK_AFFILIATE_ID
             "&url="
             "/hotels/Acapulco-Resort-Convention-Spa,-c7163-h93048-details/2023-10-03/2023-10-04/1adults",
             ());
  // Locale APIs return ISO 3166-1 "GB" for the United Kingdom, not "UK".
  TEST_EQUAL(GetKayakHotelURL("GB", 93048, "Acapulco Resort Convention Spa", 7163, 1696321580, 1696407980),
             "https://www.kayak.co.uk/in?a=" KAYAK_AFFILIATE_ID
             "&url="
             "/hotels/Acapulco-Resort-Convention-Spa,-c7163-h93048-details/2023-10-03/2023-10-04/1adults",
             ());
  TEST_EQUAL(
      GetKayakHotelURLFromURI("TR", "Elexus Hotel Resort & Spa & Casino,-c7163-h2651619", 1696321580, 1696407980),
      "https://www.kayak.com.tr/in?a=" KAYAK_AFFILIATE_ID
      "&url="
      "/hotels/Elexus-Hotel-Resort--Spa--Casino,-c7163-h2651619-details/2023-10-03/2023-10-04/1adults",
      ());
  TEST_EQUAL(
      GetKayakHotelURLFromURI("TR", "Elexus Hotel Resort & Spa & Casino,-c7163-h2651619crap", 1696321580, 1696407980),
      "", ());
  TEST_EQUAL(
      GetKayakHotelURLFromURI("TR", "Elexus Hotel Resort & Spa & Casino,-c7crap163-h2651619", 1696321580, 1696407980),
      "", ());
  TEST_EQUAL(GetKayakHotelURLFromURI("TR", "Elexus Hotel Resort & Spa & Casino,-c7163", 1696321580, 1696407980), "",
             ());
  TEST_EQUAL(GetKayakHotelURLFromURI("TR", "Elexus Hotel Resort & Spa & Casino,-h2651619", 1696321580, 1696407980), "",
             ());
  TEST_EQUAL(GetKayakHotelURLFromURI("TR", "Elexus Hotel Resort & Spa & Casino", 1696321580, 1696407980), "", ());
  TEST_EQUAL(GetKayakHotelURLFromURI("TR", "-h ,-c", 1696321580, 1696407980), "", ());
  TEST_EQUAL(GetKayakHotelURLFromURI("TR", "", 1696321580, 1696407980), "", ());
}
