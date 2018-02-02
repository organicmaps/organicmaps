#pragma once

#include "partners_api/booking_availability_params.hpp"

#include "platform/safe_callback.hpp"

#include <chrono>
#include <functional>
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
// NOTE: this callback will be called on the network thread.
using GetHotelAvailabilityCallback = std::function<void(std::vector<std::string> hotelIds)>;

/// Callbacks will be called in the same order as methods are called.
class Api
{
public:
  std::string GetBookHotelUrl(std::string const & baseUrl) const;
  std::string GetDeepLink(std::string const & hotelId) const;
  std::string GetDescriptionUrl(std::string const & baseUrl) const;
  std::string GetHotelReviewsUrl(std::string const & hotelId, std::string const & baseUrl) const;
  std::string GetSearchUrl(std::string const & city, std::string const & name) const;
  std::string ApplyAvailabilityParams(std::string const & url, AvailabilityParams const & params);

  /// Real-time information methods (used for retrieving rapidly changing information).
  /// These methods send requests directly to Booking.
  void GetMinPrice(std::string const & hotelId, std::string const & currency,
                   GetMinPriceCallback const & fn) const;

  /// NOTE: callback will be called on the network thread.
  void GetHotelAvailability(AvailabilityParams const & params,
                            GetHotelAvailabilityCallback const & fn) const;

  /// Static information methods (use for information that can be cached).
  /// These methods use caching server to prevent Booking from being ddossed.
  void GetHotelInfo(std::string const & hotelId, std::string const & lang,
                    GetHotelInfoCallback const & fn) const;
};

void SetBookingUrlForTesting(std::string const & url);
}  // namespace booking
