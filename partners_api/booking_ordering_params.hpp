#pragma once

#include "coding/url.hpp"

#include <chrono>
#include <vector>

namespace booking
{
struct OrderingParams
{
  struct Room
  {
    Room() = default;
    Room(uint8_t adultsCount, std::vector<int8_t> const & ageOfChildren);

    void SetAdultsCount(uint8_t adultsCount);
    void SetAgeOfChildren(std::vector<int8_t> const & ageOfChildren);

    uint8_t GetAdultsCount() const;
    std::vector<int8_t> const & GetAgeOfChildren() const;

    std::string ToString() const;

    bool operator!=(Room const & rhs) const;
    bool operator==(Room const & rhs) const;

  private:
    uint8_t m_adultsCount = 0;
    // No children by default.
    std::vector<int8_t> m_ageOfChildren;
  };

  using Clock = std::chrono::system_clock;
  using Time = Clock::time_point;
  using Rooms = std::vector<Room>;

  bool IsEmpty() const;
  bool Equals(OrderingParams const & rhs) const;

  url::Params Get() const;

  static OrderingParams MakeDefault();
  static OrderingParams MakeDefaultMinPrice();

  /// The arrival date. Must be within 360 days in the future and in the format yyyy-mm-dd.
  Time m_checkin;
  /// The departure date. Must be later than [m_checkin]. Must be between 1 and 30 days after
  /// [m_checkin]. Must be within 360 days in the future and in the format yyyy-mm-dd.
  Time m_checkout;
  /// Each room is comma-separated array of guests to stay in this room where "A" represents an
  /// adult and an integer represents a child. eg room1=A,A,4 would be a room with 2 adults and 1
  /// four year-old child. Child age numbers are 0..17.
  Rooms m_rooms;
};
}  // namespace booking
