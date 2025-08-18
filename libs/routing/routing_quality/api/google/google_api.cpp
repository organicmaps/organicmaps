#include "routing/routing_quality/api/google/google_api.hpp"

#include "platform/http_client.hpp"

#include "coding/serdes_json.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <chrono>
#include <ctime>
#include <sstream>
#include <utility>

namespace
{
auto GetNextDayAtNight(int32_t startTimeZoneUTC)
{
  auto now = std::chrono::system_clock::now();

  std::time_t nowTimeT = std::chrono::system_clock::to_time_t(now);
  std::tm * date = std::localtime(&nowTimeT);

  std::time_t constexpr kSecondsInDay = 24 * 60 * 60;
  std::time_t nextDay = std::mktime(date) + kSecondsInDay;

  std::tm * nextDayDate = std::localtime(&nextDay);

  long constexpr kSecondsInHour = 3600;
  int const curUTCOffset = static_cast<int>(nextDayDate->tm_gmtoff / kSecondsInHour);

  nextDayDate->tm_sec = 0;
  nextDayDate->tm_min = 0;

  int constexpr kStartHour = 22;
  // If in Moscow (UTC+3) it is 20:00, in New York (UTC-5):
  // 20:00 - 03:00 (Moscow) = 19:00 (UTC+0), so in New York: 19:00 - 05:00 = 14:00 (UTC-5).
  // Thus if we want to find such time (Y hours) in Moscow (UTC+3) so that in New York (UTC-5)
  // it's must be X hours, we use such formula:
  // 1) Y (UTC+3) - 03:00 = UTC+0
  // 2) Y (UTC+3) - 03:00 + 05:00 = X (UTC+5)
  // 2 => Y = X + 03:00 - 05:00
  // For google api we want to find such Y, that it's will be |kStartHour| hours in any place.
  nextDayDate->tm_hour = kStartHour + curUTCOffset - startTimeZoneUTC;

  return std::chrono::system_clock::from_time_t(std::mktime(nextDayDate));
}
}  // namespace

namespace routing_quality
{
namespace api
{
namespace google
{
// static
std::string const GoogleApi::kApiName = "google";

GoogleApi::GoogleApi(std::string const & token) : RoutingApi(kApiName, token, kMaxRPS) {}

Response GoogleApi::CalculateRoute(Params const & params, int32_t startTimeZoneUTC) const
{
  Response response;
  GoogleResponse googleResponse = MakeRequest(params, startTimeZoneUTC);

  response.m_params = params;
  response.m_code = googleResponse.m_code;
  for (auto const & route : googleResponse.m_routes)
  {
    CHECK_EQUAL(route.m_legs.size(), 1, ("No waypoints support for google api."));
    auto const & leg = route.m_legs.back();

    api::Route apiRoute;
    apiRoute.m_eta = leg.m_duration.m_seconds;
    apiRoute.m_distance = leg.m_distance.m_meters;

    auto const startLatLon = leg.m_steps.front().m_startLocation;
    apiRoute.m_waypoints.emplace_back(startLatLon.m_lat, startLatLon.m_lon);
    for (auto const & step : leg.m_steps)
    {
      auto const & prev = step.m_startLocation;
      auto const & next = step.m_endLocation;

      auto const & prevWaypoint = apiRoute.m_waypoints.back();
      CHECK(prevWaypoint.m_lat == prev.m_lat && prevWaypoint.m_lon == prev.m_lon, ());

      apiRoute.m_waypoints.emplace_back(next.m_lat, next.m_lon);
    }

    response.m_routes.emplace_back(std::move(apiRoute));
  }

  return response;
}

GoogleResponse GoogleApi::MakeRequest(Params const & params, int32_t startTimeZoneUTC) const
{
  GoogleResponse googleResponse;
  platform::HttpClient request(GetDirectionsURL(params, startTimeZoneUTC));

  if (request.RunHttpRequest() && !request.WasRedirected() && request.ErrorCode() == 200)
  {
    auto const & response = request.ServerResponse();
    CHECK(!response.empty(), ());
    {
      googleResponse.m_code = ResultCode::ResponseOK;
      coding::DeserializerJson des(response);
      des(googleResponse);
    }
  }
  else
  {
    LOG(LWARNING, (request.ErrorCode(), request.ServerResponse()));
    googleResponse.m_code = ResultCode::Error;
  }

  return googleResponse;
}

std::string GoogleApi::GetDirectionsURL(Params const & params, int32_t startTimeZoneUTC) const
{
  CHECK(-12 <= startTimeZoneUTC && startTimeZoneUTC <= 12, ("Invalid UTC zone."));

  static std::string const kBaseUrl = "https://maps.googleapis.com/maps/api/directions/json?";

  auto const start = mercator::ToLatLon(params.m_waypoints.GetPointFrom());
  auto const finish = mercator::ToLatLon(params.m_waypoints.GetPointTo());

  auto const nextDayAtNight = GetNextDayAtNight(startTimeZoneUTC);
  auto const secondFromEpoch = nextDayAtNight.time_since_epoch().count() / 1000000;
  LOG(LDEBUG, ("&departure_time =", secondFromEpoch));

  std::stringstream ss;
  ss << kBaseUrl << "&origin=" << std::to_string(start.m_lat) << "," << std::to_string(start.m_lon)
     << "&destination=" << std::to_string(finish.m_lat) << "," << std::to_string(finish.m_lon)
     << "&key=" << GetAccessToken() << "&alternatives=true"
     << "&departure_time=" << secondFromEpoch;

  return ss.str();
}
}  // namespace google
}  // namespace api
}  // namespace routing_quality
