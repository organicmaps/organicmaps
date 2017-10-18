#pragma once

#include "platform/safe_callback.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace booking
{
struct HotelPhotoUrls
{
  std::string m_small;
  std::string m_original;
};

struct HotelReview
{
  /// An issue date.
  std::chrono::time_point<std::chrono::system_clock> m_date;
  /// Author's hotel evaluation.
  float m_score = 0.0;
  /// Review author name.
  std::string m_author;
  /// Review text. There can be either one or both positive/negative review.
  std::string m_pros;
  std::string m_cons;
};

struct HotelFacility
{
  std::string m_type;
  std::string m_name;
};

struct HotelInfo
{
  std::string m_hotelId;

  std::string m_description;
  std::vector<HotelPhotoUrls> m_photos;
  std::vector<HotelFacility> m_facilities;
  std::vector<HotelReview> m_reviews;
  float m_score = 0.0;
  uint32_t m_scoreCount = 0;
};

/// Params for checking availability of hotels.
/// [m_hotelIds], [m_checkin], [m_checkout], [m_rooms] are required.
struct AvailabilityParams
{
  using Time = std::chrono::system_clock::time_point;

  std::vector<std::pair<std::string, std::string>> Get() const;

  /// Limit the result list to the specified hotels where they have availability for the
  /// specified guests and dates.
  std::vector<std::string> m_hotelIds;
  /// The arrival date. Must be within 360 days in the future and in the format yyyy-mm-dd.
  Time m_checkin;
  /// The departure date. Must be later than [m_checkin]. Must be between 1 and 30 days after
  /// [m_checkin]. Must be within 360 days in the future and in the format yyyy-mm-dd.
  Time m_checkout;
  /// Each room is s comma separated array of guests to stay in this room where "A" represents an
  /// adult and an integer represents a child. eg room1=A,A,4 would be a room with 2 adults and 1
  /// four year-old child. Child age numbers are 0..17.
  std::vector<std::string> m_rooms;
  /// Show only hotels with review_score >= that. min_review_score should be in the range 1 to 10.
  /// Values are rounded down: min_review_score 7.8 will result in properties with review scores
  /// of 7 and up.
  double m_minReviewScore = {};
  /// Limit to hotels with the given number(s) of stars. Supported values 1-5.
  vector<std::string> m_stars;
};

class RawApi
{
public:
  // Booking Api v1 methods:
  static bool GetHotelAvailability(std::string const & hotelId, std::string const & currency, std::string & result);
  static bool GetExtendedInfo(std::string const & hotelId, std::string const & lang, std::string & result);
  // Booking Api v2 methods:
  static bool HotelAvailability(AvailabilityParams const & params, std::string & result);
};

using GetMinPriceCallback = platform::SafeCallback<void(std::string const & hotelId, std::string const & price, std::string const & currency)>;
using GetHotelInfoCallback = platform::SafeCallback<void(HotelInfo const & hotelInfo)>;
using GetHotelAvailabilityCallback = platform::SafeCallback<void(std::vector<uint64_t> hotelIds)>;

class Api
{
public:
  std::string GetBookHotelUrl(std::string const & baseUrl) const;
  std::string GetDescriptionUrl(std::string const & baseUrl) const;
  std::string GetHotelReviewsUrl(std::string const & hotelId, std::string const & baseUrl) const;
  std::string GetSearchUrl(std::string const & city, std::string const & name) const;
  /// Real-time information methods (used for retriving rapidly changing information).
  /// These methods send requests directly to Booking.
  void GetMinPrice(std::string const & hotelId, std::string const & currency, GetMinPriceCallback const & fn);

  /// Static information methods (use for information that can be cached).
  /// These methods use caching server to prevent Booking from being ddossed.
  void GetHotelInfo(std::string const & hotelId, std::string const & lang, GetHotelInfoCallback const & fn);

  void GetHotelAvailability(AvailabilityParams const & params,
                            GetHotelAvailabilityCallback const & fn);
};

void SetBookingUrlForTesting(std::string const & url);
}  // namespace booking
