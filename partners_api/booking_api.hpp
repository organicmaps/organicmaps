#pragma once

#include "std/chrono.hpp"
#include "std/function.hpp"
#include "std/shared_ptr.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

namespace booking
{
struct HotelPhotoUrls
{
  string m_small;
  string m_original;
};

struct HotelReview
{
  /// An issue date.
  time_point<system_clock> m_date;
  /// Author's hotel evaluation.
  float m_score = 0.0;
  /// Review author name.
  string m_author;
  /// Review text. There can be either one or both positive/negative review.
  string m_pros;
  string m_cons;
};

struct HotelFacility
{
  string m_type;
  string m_name;
};

struct HotelInfo
{
  string m_hotelId;

  string m_description;
  vector<HotelPhotoUrls> m_photos;
  vector<HotelFacility> m_facilities;
  vector<HotelReview> m_reviews;
  float m_score = 0.0;
  uint32_t m_scoreCount = 0;
};

class RawApi
{
public:
  static bool GetHotelAvailability(string const & hotelId, string const & currency, string & result);
  static bool GetExtendedInfo(string const & hotelId, string const & lang, string & result);
};

using GetMinPriceCallback = function<void(string const & hotelId, string const & price, string const & currency)>;
using GetHotelInfoCallback = function<void(HotelInfo const & hotelInfo)>;

class Api
{
public:
  string GetBookHotelUrl(string const & baseUrl) const;
  string GetDescriptionUrl(string const & baseUrl) const;
  string GetHotelReviewsUrl(string const & hotelId, string const & baseUrl) const;
  // Real-time information methods (used for retriving rapidly changing information).
  // These methods send requests directly to Booking.
  void GetMinPrice(string const & hotelId, string const & currency, GetMinPriceCallback const & fn);

  // Static information methods (use for information that can be cached).
  // These methods use caching server to prevent Booking from being ddossed.
  void GetHotelInfo(string const & hotelId, string const & lang, GetHotelInfoCallback const & fn);
};

void SetBookingUrlForTesting(string const & url);
}  // namespace booking
