#include "partners_api/booking_availability_params.hpp"
#include "partners_api/utils.hpp"

#include "base/string_utils.hpp"

#include <sstream>

using namespace base;

namespace
{
std::string FormatTime(booking::AvailabilityParams::Time p)
{
  return partners_api::FormatTime(p, "%Y-%m-%d");
}

bool Contains(std::unordered_set<std::string> const & filter, std::string const & value)
{
  return filter.find(value) != filter.cend();
}
}  // namespace

namespace booking
{
AvailabilityParams::Room::Room(uint8_t adultsCount, int8_t ageOfChild)
  : m_adultsCount(adultsCount), m_ageOfChild(ageOfChild)
{
}

void AvailabilityParams::Room::SetAdultsCount(uint8_t adultsCount)
{
  m_adultsCount = adultsCount;
}

void AvailabilityParams::Room::SetAgeOfChild(int8_t ageOfChild)
{
  m_ageOfChild = ageOfChild;
}

int8_t AvailabilityParams::Room::GetAdultsCount() const
{
  return m_adultsCount;
}

int8_t AvailabilityParams::Room::GetAgeOfChild() const
{
  return m_ageOfChild;
}

std::string AvailabilityParams::Room::ToString() const
{
  static std::string const kAdult = "A";
  std::vector<std::string> adults(m_adultsCount, kAdult);
  std::string child = m_ageOfChild == kNoChildren ? "" : "," + std::to_string(m_ageOfChild);

  std::ostringstream os;
  os << strings::JoinStrings(adults, ',') << child;

  return os.str();
}

bool AvailabilityParams::Room::operator!=(AvailabilityParams::Room const & rhs) const
{
  return m_adultsCount != rhs.m_adultsCount || m_ageOfChild != rhs.m_ageOfChild;
}

bool AvailabilityParams::Room::operator==(AvailabilityParams::Room const & rhs) const
{
  return !this->operator!=(rhs);
}

url::Params AvailabilityParams::Get(std::unordered_set<std::string> const & filter /* = {} */) const
{
  url::Params result;

  if (filter.empty() || Contains(filter, "hotel_ids"))
    result.emplace_back("hotel_ids", strings::JoinStrings(m_hotelIds, ','));

  if (filter.empty() || Contains(filter, "checkin"))
    result.emplace_back("checkin", FormatTime(m_checkin));

  if (filter.empty() || Contains(filter, "checkout"))
    result.emplace_back("checkout", FormatTime(m_checkout));

  if (filter.empty() || Contains(filter, "room"))
  {
    for (size_t i = 0; i < m_rooms.size(); ++i)
      result.emplace_back("room" + to_string(i + 1), m_rooms[i].ToString());
  }

  if (m_minReviewScore != 0.0 && (filter.empty() || Contains(filter, "min_review_score")))
    result.emplace_back("min_review_score", to_string(m_minReviewScore));

  if (!m_stars.empty() && (filter.empty() || Contains(filter, "stars")))
    result.emplace_back("stars", strings::JoinStrings(m_stars, ','));

  return result;
}

bool AvailabilityParams::IsEmpty() const
{
  return m_checkin == Time() || m_checkout == Time() || m_rooms.empty();
}

bool AvailabilityParams::operator!=(AvailabilityParams const & rhs) const
{
  return m_checkin != rhs.m_checkin || m_checkout != rhs.m_checkout || m_rooms != rhs.m_rooms ||
         m_minReviewScore != rhs.m_minReviewScore || m_stars != rhs.m_stars;
}
bool AvailabilityParams::operator==(AvailabilityParams const & rhs) const
{
  return !(*this != rhs);
}
}  // namespace booking
