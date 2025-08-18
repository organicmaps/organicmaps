#include "testing/testing.hpp"

#include "glaze/json.hpp"

#include <optional>
#include <string>

enum class LocationSource
{
  GPS,
  Network,
  Unknown
};

// If enumeration should be represented as string instead of integer in JSON,
// then enum should be declared in the global namespace.
template <>
struct glz::meta<LocationSource>
{
  using enum LocationSource;
  static constexpr auto value = glz::enumerate(GPS, Network, Unknown);
};

// For our test macros.
std::string DebugPrint(LocationSource source)
{
  return std::string{glz::get_enum_name(source)};
}

namespace glaze_test
{
struct Location
{
  LocationSource source;
  double latitude;
  double longitude;
  double accuracy;
  std::optional<double> altitude;
  std::optional<double> altitudeAccuracy;
};

struct AccessToken
{
  Location location;
  std::string accessToken;
};

UNIT_TEST(Glaze_Smoke)
{
  std::string_view constexpr fullJsonWithComments = R"({
  "location": {
    "source": "GPS",  // Where the location data comes from.
    "latitude": 47.3345141,
    "longitude": 8.5312839,
    "accuracy": 22,
    "altitude": 100.5,
    "altitudeAccuracy": 5.0
  },
  /* Access token for authentication
   * This token is used to access protected resources.
   */
  "accessToken": "2:vC65Xv0mxMtsNVf4:hY5YSIkuFfnAU77z"
})";

  AccessToken token;
  auto error = glz::read_jsonc(token, fullJsonWithComments);
  TEST(!error, (glz::format_error(error, fullJsonWithComments)));

  TEST_EQUAL(token.location.source, LocationSource::GPS, ());
  TEST_EQUAL(token.location.latitude, 47.3345141, ());
  TEST_EQUAL(token.location.longitude, 8.5312839, ());
  TEST_EQUAL(token.location.accuracy, 22.0, ());
  TEST_EQUAL(token.location.altitude, 100.5, ());
  TEST_EQUAL(token.location.altitudeAccuracy, 5.0, ());
  TEST_EQUAL(token.accessToken, "2:vC65Xv0mxMtsNVf4:hY5YSIkuFfnAU77z", ());

  std::string_view constexpr partialJson =
      R"({"location":{"source":"Network","latitude":47.3345141,"longitude":8.5312839,"accuracy":22},"accessToken":""})";

  token.location.source = LocationSource::Network;
  token.location.altitude = {};
  token.location.altitudeAccuracy = {};
  token.accessToken = {};

  std::string buffer;
  error = glz::write_json(token, buffer);
  TEST(!error, (glz::format_error(error, "Failed to write JSON")));
  TEST_EQUAL(buffer, partialJson, ());
}
}  // namespace glaze_test
