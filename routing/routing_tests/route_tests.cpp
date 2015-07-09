#include "testing/testing.hpp"

#include "routing/route.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"

using namespace routing;

UNIT_TEST(AddAdsentCountryToRouteTest)
{
  Route route("TestRouter");
  route.AddAbsentCountry("A");
  route.AddAbsentCountry("A");
  route.AddAbsentCountry("B");
  route.AddAbsentCountry("C");
  route.AddAbsentCountry("B");
  set<string> const & absent = route.GetAbsentCountries();
  TEST(absent.find("A") != absent.end(), ());
  TEST(absent.find("B") != absent.end(), ());
  TEST(absent.find("C") != absent.end(), ());
}
