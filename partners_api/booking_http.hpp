#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace booking
{
namespace http
{
bool RunSimpleHttpRequest(bool const needAuth, std::string const & url, std::string & result);

using Time = std::chrono::system_clock::time_point;
using Params = std::vector<std::pair<std::string, std::string>>;
using Hotels = std::vector<std::string>;
using Rooms = std::vector<std::string>;
using Stars = std::vector<std::string>;

std::string MakeApiUrl(std::string const & baseUrl, std::string const & func, Params const & params);
std::string FormatTime(Time p);

/// Params for checking availability of hotels.
/// [m_hotelIds], [m_checkin], [m_checkout], [m_rooms] are required.
struct AvailabilityParams
{
  Params Get() const;

  /// Limit the result list to the specified hotels where they have availability for the
  /// specified guests and dates.
  Hotels m_hotelIds;
  /// The arrival date. Must be within 360 days in the future and in the format yyyy-mm-dd.
  Time m_checkin;
  /// The departure date. Must be later than [m_checkin]. Must be between 1 and 30 days after
  /// [m_checkin]. Must be within 360 days in the future and in the format yyyy-mm-dd.
  Time m_checkout;
  /// Each room is s comma separated array of guests to stay in this room where "A" represents an
  /// adult and an integer represents a child. eg room1=A,A,4 would be a room with 2 adults and 1
  /// four year-old child. Child age numbers are 0..17.
  Rooms m_rooms;
  /// Show only hotels with review_score >= that. min_review_score should be in the range 1 to 10.
  /// Values are rounded down: min_review_score 7.8 will result in properties with review scores
  /// of 7 and up.
  double m_minReviewScore = {};
  /// Limit to hotels with the given number(s) of stars. Supported values 1-5.
  Stars m_stars;
};
}  // namespace http
}  // namespace booking
