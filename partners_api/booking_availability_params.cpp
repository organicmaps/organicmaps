#include "partners_api/booking_availability_params.hpp"

#include "base/string_utils.hpp"

#include <sstream>

using namespace base;

namespace
{
bool IsAcceptedByFilter(booking::AvailabilityParams::UrlFilter const & filter,
                        std::string const & value)
{
  if (filter.empty())
    return true;

  return filter.find(value) != filter.cend();
}
}  // namespace

namespace booking
{
AvailabilityParams::Room::Room(uint8_t adultsCount, std::vector<int8_t> const & ageOfChildren)
  : m_adultsCount(adultsCount), m_ageOfChildren(ageOfChildren)
{
}

void AvailabilityParams::Room::SetAdultsCount(uint8_t adultsCount)
{
  m_adultsCount = adultsCount;
}

void AvailabilityParams::Room::SetAgeOfChildren(std::vector<int8_t> const & ageOfChildren)
{
  m_ageOfChildren = ageOfChildren;
}

uint8_t AvailabilityParams::Room::GetAdultsCount() const
{
  return m_adultsCount;
}

std::vector<int8_t> const & AvailabilityParams::Room::GetAgeOfChildren() const
{
  return m_ageOfChildren;
}

std::string AvailabilityParams::Room::ToString() const
{
  static std::string const kAdult = "A";
  std::vector<std::string> adults(m_adultsCount, kAdult);
  std::string children =
      m_ageOfChildren.empty() ? "" : "," + strings::JoinAny(m_ageOfChildren, ',', strings::ToStringConverter<int>());

  std::ostringstream os;
  os << strings::JoinStrings(adults, ',') << children;

  return os.str();
}

bool AvailabilityParams::Room::operator!=(AvailabilityParams::Room const & rhs) const
{
  return m_adultsCount != rhs.m_adultsCount || m_ageOfChildren != rhs.m_ageOfChildren;
}

bool AvailabilityParams::Room::operator==(AvailabilityParams::Room const & rhs) const
{
  return !this->operator!=(rhs);
}

// static
AvailabilityParams AvailabilityParams::MakeDefault()
{
  AvailabilityParams result;
  // Use tomorrow and day after tomorrow by default.
  result.m_checkin = Clock::now();
  result.m_checkout = Clock::now() + std::chrono::hours(24);
  // Use two adults without children.
  result.m_rooms = {{2, {}}};

  return result;
}

url::Params AvailabilityParams::Get(UrlFilter const & filter /* = {} */) const
{
  url::Params result;

  if (IsAcceptedByFilter(filter, "hotel_ids"))
    result.emplace_back("hotel_ids", strings::JoinStrings(m_hotelIds, ','));

  if (IsAcceptedByFilter(filter, "checkin"))
    result.emplace_back("checkin", FormatTime(m_checkin));

  if (IsAcceptedByFilter(filter, "checkout"))
    result.emplace_back("checkout", FormatTime(m_checkout));

  if (IsAcceptedByFilter(filter, "room"))
  {
    for (size_t i = 0; i < m_rooms.size(); ++i)
      result.emplace_back("room" + std::to_string(i + 1), m_rooms[i].ToString());
  }

  if (m_minReviewScore != 0.0 && IsAcceptedByFilter(filter, "min_review_score"))
    result.emplace_back("min_review_score", std::to_string(m_minReviewScore));

  if (!m_stars.empty() && IsAcceptedByFilter(filter, "stars"))
    result.emplace_back("stars", strings::JoinStrings(m_stars, ','));

  if (m_dealsOnly)
    result.emplace_back("show_only_deals", "smart,lastm");

  return result;
}

bool AvailabilityParams::IsEmpty() const
{
  return m_checkin == Time() || m_checkout == Time() || m_rooms.empty();
}

bool AvailabilityParams::Equals(ParamsBase const & rhs) const
{
  return rhs.Equals(*this);
}

bool AvailabilityParams::Equals(AvailabilityParams const & rhs) const
{
  return m_checkin == rhs.m_checkin && m_checkout == rhs.m_checkout && m_rooms == rhs.m_rooms &&
         m_minReviewScore == rhs.m_minReviewScore && m_stars == rhs.m_stars &&
         m_dealsOnly == rhs.m_dealsOnly;
}

void AvailabilityParams::Set(ParamsBase const & src)
{
  src.CopyTo(*this);
}
}  // namespace booking
