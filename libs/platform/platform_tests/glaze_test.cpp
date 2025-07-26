#include "testing/testing.hpp"

#include "glaze/glaze.hpp"

#include <string>

namespace glaze_test
{
struct Location
{
  double latitude;
  double longitude;
  double accuracy;
};
struct AccessToken
{
  Location location = {.latitude = 47.3345141, .longitude = 8.5312839, .accuracy = 22.};
  std::string access_token = "2:vC65Xv0mxMtsNVf4:hY5YSIkuFfnAU77z";
};

UNIT_TEST(Glaze_Smoke)
{
  std::string_view constexpr json =
      "{\"location\":{\"latitude\":47.3345141,\"longitude\":8.5312839,"
      "\"accuracy\":22},\"access_token\":\"2:vC65Xv0mxMtsNVf4:hY5YSIkuFfnAU77z\"}";

  AccessToken token;

  std::string buffer;
  auto ec = glz::write_json(token, buffer);
  TEST(!ec, ("Failed to write JSON:", ec));
  TEST(buffer == json, ("buffer does not match json:", buffer, json));

  token.location = {.latitude = 0.0, .longitude = 0.0, .accuracy = 0.0};
  token.access_token.clear();

  ec = glz::read_json(token, json);
  TEST(!ec, ("Failed to read JSON:", ec));
  TEST_EQUAL(token.location.latitude, 47.3345141, ("Latitude does not match"));
  TEST_EQUAL(token.location.longitude, 8.5312839, ("Longitude does not match"));
  TEST_EQUAL(token.location.accuracy, 22.0, ("Accuracy does not match"));
  TEST_EQUAL(token.access_token, "2:vC65Xv0mxMtsNVf4:hY5YSIkuFfnAU77z", ("Access token does not match"));
}
}  // namespace glaze_test
