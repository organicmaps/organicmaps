#pragma once

#include "partners_api/booking_ordering_params.hpp"
#include "partners_api/booking_params_base.hpp"

#include "coding/url.hpp"

#include <chrono>
#include <string>
#include <unordered_set>
#include <vector>

namespace booking
{
/// Params for checking availability of hotels.
/// [m_hotelIds], [m_checkin], [m_checkout], [m_rooms] are required.
struct AvailabilityParams : public ParamsBase
{
  using Hotels = std::vector<std::string>;
  using Stars = std::vector<std::string>;

  static AvailabilityParams MakeDefault();

  using UrlFilter = std::unordered_set<std::string>;
  url::Params Get(UrlFilter const & filter = {}) const;

  // ParamsBase overrides:
  bool IsEmpty() const override;
  bool Equals(ParamsBase const & rhs) const override;
  bool Equals(AvailabilityParams const & rhs) const override;
  void Set(ParamsBase const & src) override;

  /// Limit the result list to the specified hotels where they have availability for the
  /// specified guests and dates.
  Hotels m_hotelIds;

  // Check-in/check-out dates and rooms.
  // For detailed description see booking::OrderingParams declaration.
  OrderingParams m_orderingParams;
  /// Show only hotels with review_score >= that. min_review_score should be in the range 1 to 10.
  /// Values are rounded down: min_review_score 7.8 will result in properties with review scores
  /// of 7 and up.
  double m_minReviewScore = {};
  /// Limit to hotels with the given number(s) of stars. Supported values 1-5.
  Stars m_stars;
  /// Only show rates that are deals of types: smart, lastm.
  bool m_dealsOnly = false;
};
}  // namespace booking
