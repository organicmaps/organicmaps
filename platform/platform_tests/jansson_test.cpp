#include "../../testing/testing.hpp"

#include <myjansson.hpp>

UNIT_TEST(Jansson_Smoke)
{
  char const * str = "{\"location\":{\"latitude\":47.383333,\"longitude\":8.533333,"
      "\"accuracy\":18000.0},\"access_token\":\"2:6aOjM2IAoPMaweWN:txhu5LpkRkLVb3u3\"}";

  my::Json root(str);
  TEST(json_is_object(root), ());
  json_t * location = json_object_get(root, "location");
  TEST(json_is_object(location), ());

  json_t * lat = json_object_get(location, "latitude");
  TEST(json_is_real(lat), ());
  TEST_ALMOST_EQUAL(json_real_value(lat), 47.383333, ());

  json_t * lon = json_object_get(location, "longitude");
  TEST(json_is_real(lon), ());
  TEST_ALMOST_EQUAL(json_real_value(lon), 8.533333, ());

  json_t * acc = json_object_get(location, "accuracy");
  TEST(json_is_real(acc), ());
  TEST_ALMOST_EQUAL(json_real_value(acc), 18000.0, ());
}
