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
// static
AvailabilityParams AvailabilityParams::MakeDefault()
{
  AvailabilityParams result;
  result.m_orderingParams = OrderingParams::MakeDefault();

  return result;
}

url::Params AvailabilityParams::Get(UrlFilter const & filter /* = {} */) const
{
  url::Params result = m_orderingParams.Get();

  if (IsAcceptedByFilter(filter, "hotel_ids"))
    result.emplace_back("hotel_ids", strings::JoinStrings(m_hotelIds, ','));

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
  return m_orderingParams.IsEmpty();
}

bool AvailabilityParams::Equals(ParamsBase const & rhs) const
{
  return rhs.Equals(*this);
}

bool AvailabilityParams::Equals(AvailabilityParams const & rhs) const
{
  return m_orderingParams.Equals(rhs.m_orderingParams) &&
         m_minReviewScore == rhs.m_minReviewScore && m_stars == rhs.m_stars &&
         m_dealsOnly == rhs.m_dealsOnly;
}

void AvailabilityParams::Set(ParamsBase const & src)
{
  src.CopyTo(*this);
}
}  // namespace booking
