#pragma once

#include "partners_api/booking_availability_params.hpp"
#include "partners_api/booking_block_params.hpp"

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

struct Deals
{
  enum class Type
  {
    /// Good price.
    Smart,
    /// Sale with discount in percent from base price.
    LastMinute
  };

  std::vector<Type> m_types;
  uint8_t m_discount = 0;
};

struct BlockInfo
{
  static double constexpr kIncorrectPrice = std::numeric_limits<double>::max();
  std::string m_blockId;
  std::string m_name;
  std::string m_description;
  uint8_t m_maxOccupancy = 0;
  double m_minPrice = kIncorrectPrice;
  std::string m_currency;
  std::vector<std::string> m_photos;
  Deals m_deals;
  std::chrono::time_point<std::chrono::system_clock> m_refundableUntil;
  bool m_breakfastIncluded = false;
  bool m_depositRequired = false;
};

struct Blocks
{
  void Add(BlockInfo && block)
  {
    if (block.m_minPrice < m_totalMinPrice)
    {
      m_totalMinPrice = block.m_minPrice;
      m_currency = block.m_currency;
    }
    if (!m_hasSmartDeal)
    {
      auto const & types = block.m_deals.m_types;
      m_hasSmartDeal = std::find(types.cbegin(), types.cend(), Deals::Type::Smart) != types.cend();
    }
    if (block.m_deals.m_discount > m_maxDiscount)
      m_maxDiscount = block.m_deals.m_discount;

    m_blocks.emplace_back(block);
  }

  double m_totalMinPrice = BlockInfo::kIncorrectPrice;
  std::string m_currency;

  uint8_t m_maxDiscount = 0;
  bool m_hasSmartDeal = false;

  std::vector<BlockInfo> m_blocks;
};

class RawApi
{
public:
  // Booking Api v1 methods:
  static bool GetHotelAvailability(std::string const & hotelId, std::string const & currency, std::string & result);
  static bool GetExtendedInfo(std::string const & hotelId, std::string const & lang, std::string & result);
  // Booking Api v2 methods:
  static bool HotelAvailability(AvailabilityParams const & params, std::string & result);
  static bool BlockAvailability(BlockParams const & params, string & result);
};

using BlockAvailabilityCallback =
    platform::SafeCallback<void(std::string const & hotelId, Blocks const & blocks)>;
using GetHotelInfoCallback = platform::SafeCallback<void(HotelInfo const & hotelInfo)>;
// NOTE: this callback will be called on the network thread.
using GetHotelAvailabilityCallback = std::function<void(std::vector<std::string> hotelIds)>;

/// This is a lightweight class but methods are non-static in order to support the NetworkPolicy
/// restrictions.
/// Callbacks will be called in the same order as methods are called.
class Api
{
public:
  std::string GetBookHotelUrl(std::string const & baseUrl) const;
  std::string GetDeepLink(std::string const & hotelId) const;
  std::string GetDescriptionUrl(std::string const & baseUrl) const;
  std::string GetHotelReviewsUrl(std::string const & hotelId, std::string const & baseUrl) const;
  std::string GetSearchUrl(std::string const & city, std::string const & name) const;
  std::string ApplyAvailabilityParams(std::string const & url,
                                      AvailabilityParams const & params) const;

  /// Real-time information methods (used for retrieving rapidly changing information).
  /// These methods send requests directly to Booking.
  void GetBlockAvailability(BlockParams && params, BlockAvailabilityCallback const & fn) const;

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
