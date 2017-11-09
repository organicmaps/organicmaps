#include "partners_api/booking_availability_params.hpp"
#include "partners_api/utils.hpp"

#include "base/string_utils.hpp"

#include "private.h"

using namespace base::url;

namespace
{
std::string FormatTime(booking::AvailabilityParams::Time p)
{
  return partners_api::FormatTime(p, "%Y-%m-%d");
}
}  // namespace

namespace booking
{
Params AvailabilityParams::Get() const
{
  Params result;

  result.push_back({"hotel_ids", strings::JoinStrings(m_hotelIds, ',')});
  result.push_back({"checkin", FormatTime(m_checkin)});
  result.push_back({"checkout", FormatTime(m_checkout)});

  for (size_t i = 0; i < m_rooms.size(); ++i)
    result.push_back({"room" + to_string(i + 1), m_rooms[i]});

  if (m_minReviewScore != 0.0)
    result.push_back({"min_review_score", to_string(m_minReviewScore)});

  if (!m_stars.empty())
    result.push_back({"stars", strings::JoinStrings(m_stars, ',')});

  return result;
}

bool AvailabilityParams::IsEmpty() const
{
  return m_checkin == Time() || m_checkout == Time() || m_rooms.empty();
}
}  // namespace booking
