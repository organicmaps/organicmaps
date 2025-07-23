#include "testing/testing.hpp"

#include "cppjansson/cppjansson.hpp"

UNIT_TEST(Jansson_Smoke)
{
  char * savedLocale = setlocale(LC_NUMERIC, "C");

  //  char const * str = "{\"location\":{\"latitude\":47.383333,\"longitude\":8.533333,"
  //      "\"accuracy\":18000.0},\"access_token\":\"2:6aOjM2IAoPMaweWN:txhu5LpkRkLVb3u3\"}";

  char const * str =
      "{\"location\":{\"latitude\":47.3345141,\"longitude\":8.5312839,"
      "\"accuracy\":22.0},\"access_token\":\"2:vC65Xv0mxMtsNVf4:hY5YSIkuFfnAU77z\"}";

  base::Json root(str);
  TEST(json_is_object(root.get()), ());
  json_t * location = json_object_get(root.get(), "location");
  TEST(json_is_object(location), ());

  json_t * lat = json_object_get(location, "latitude");
  TEST(json_is_real(lat), ());
  TEST_ALMOST_EQUAL_ULPS(json_real_value(lat), 47.3345141, ());

  json_t * lon = json_object_get(location, "longitude");
  TEST(json_is_real(lon), ());
  TEST_ALMOST_EQUAL_ULPS(json_real_value(lon), 8.5312839, ());

  json_t * acc = json_object_get(location, "accuracy");
  TEST(json_is_real(acc), ());
  TEST_ALMOST_EQUAL_ULPS(json_real_value(acc), 22.0, ());

  bool wasException = false;
  try
  {
    base::Json invalid("{asd]");
  }
  catch (base::Json::Exception const &)
  {
    wasException = true;
  }
  TEST(wasException, ());

  setlocale(LC_NUMERIC, savedLocale);
}
