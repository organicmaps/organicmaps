#include "partners_api/booking_ordering_params.hpp"

#include "partners_api/utils.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

namespace booking
{
OrderingParams::Room::Room(uint8_t adultsCount, std::vector<int8_t> const & ageOfChildren)
  : m_adultsCount(adultsCount), m_ageOfChildren(ageOfChildren)
{
}

void OrderingParams::Room::SetAdultsCount(uint8_t adultsCount)
{
  m_adultsCount = adultsCount;
}

void OrderingParams::Room::SetAgeOfChildren(std::vector<int8_t> const & ageOfChildren)
{
  m_ageOfChildren = ageOfChildren;
}

uint8_t OrderingParams::Room::GetAdultsCount() const
{
  return m_adultsCount;
}

std::vector<int8_t> const & OrderingParams::Room::GetAgeOfChildren() const
{
  return m_ageOfChildren;
}

std::string OrderingParams::Room::ToString() const
{
  static std::string const kAdult = "A";
  std::vector<std::string> adults(m_adultsCount, kAdult);
  std::string children =
    m_ageOfChildren.empty() ? "" : "," + strings::JoinAny(m_ageOfChildren, ',', strings::ToStringConverter<int>());

  std::ostringstream os;
  os << strings::JoinStrings(adults, ',') << children;

  return os.str();
}

bool OrderingParams::Room::operator!=(OrderingParams::Room const & rhs) const
{
  return m_adultsCount != rhs.m_adultsCount || m_ageOfChildren != rhs.m_ageOfChildren;
}

bool OrderingParams::Room::operator==(OrderingParams::Room const & rhs) const
{
  return !this->operator!=(rhs);
}

bool OrderingParams::IsEmpty() const
{
  return m_checkin == Time() || m_checkout == Time() || m_rooms.empty();
}

bool OrderingParams::Equals(OrderingParams const & rhs) const
{
  return m_checkin == rhs.m_checkin && m_checkout == rhs.m_checkout && m_rooms == rhs.m_rooms;
}

url::Params OrderingParams::Get() const
{
  ASSERT(!IsEmpty(), ());

  url::Params result;
  result.reserve(2 + m_rooms.size());
  result.emplace_back("checkin", partners_api::FormatTime(m_checkin, "%Y-%m-%d"));
  result.emplace_back("checkout", partners_api::FormatTime(m_checkout, "%Y-%m-%d"));
  for (size_t i = 0; i < m_rooms.size(); ++i)
    result.emplace_back("room" + std::to_string(i + 1), m_rooms[i].ToString());

  return result;
}

// static
OrderingParams OrderingParams::MakeDefault()
{
  OrderingParams result;
  // Use tomorrow and day after tomorrow by default.
  result.m_checkin = Clock::now();
  result.m_checkout = Clock::now() + std::chrono::hours(24);
  // Use two adults without children.
  result.m_rooms = {{2, {}}};

  return result;
}

OrderingParams OrderingParams::MakeDefaultMinPrice()
{
  OrderingParams result;
  // Use tomorrow and day after tomorrow by default.
  result.m_checkin = Clock::now();
  result.m_checkout = Clock::now() + std::chrono::hours(24);
  // Use one adult without children.
  result.m_rooms = {{1, {}}};

  return result;
}
}  // namespace booking
