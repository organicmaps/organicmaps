#pragma once

#include "base/url_helpers.hpp"

#include <chrono>
#include <string>
#include <vector>

namespace booking
{
/// Params for checking availability of hotels.
/// [m_hotelIds], [m_checkin], [m_checkout], [m_rooms] are required.
struct AvailabilityParams
{
  struct Room
  {
    static constexpr int8_t kNoChildren = -1;
    Room() = default;
    Room(uint8_t adultsCount, int8_t ageOfChild);

    void SetAdultsCount(uint8_t adultsCount);
    void SetAgeOfChild(int8_t ageOfChild);

    std::string ToString() const;

    bool operator!=(Room const & rhs) const;
    bool operator==(Room const & rhs) const;
  private:
    uint8_t m_adultsCount = 0;
    // No children by default.
    int8_t m_ageOfChild = kNoChildren;
  };

  using Clock = std::chrono::system_clock;
  using Time = Clock::time_point;
  using Hotels = std::vector<std::string>;
  using Rooms = std::vector<Room>;
  using Stars = std::vector<std::string>;

  base::url::Params Get() const;
  bool IsEmpty() const;
  bool operator!=(AvailabilityParams const & rhs) const;
  bool operator==(AvailabilityParams const & rhs) const;

  /// Limit the result list to the specified hotels where they have availability for the
  /// specified guests and dates.
  Hotels m_hotelIds;
  /// The arrival date. Must be within 360 days in the future and in the format yyyy-mm-dd.
  Time m_checkin;
  /// The departure date. Must be later than [m_checkin]. Must be between 1 and 30 days after
  /// [m_checkin]. Must be within 360 days in the future and in the format yyyy-mm-dd.
  Time m_checkout;
  /// Each room is comma-separated array of guests to stay in this room where "A" represents an
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
}  // namespace booking
