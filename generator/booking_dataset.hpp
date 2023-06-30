#pragma once

#include "generator/sponsored_dataset.hpp"
#include "generator/sponsored_object_base.hpp"

#include <string>

namespace generator
{
class BookingHotel : public SponsoredObjectBase
{
  enum Fields
  {
    Id = 0,
    Latitude,
    Longitude,
    Name,
    Address,
    Stars,
    PriceCategory,
    RatingBooking,
    RatingUsers,
    DescUrl,
    Type,
    Translations,

    Counter
  };

public:
  explicit BookingHotel(std::string src);

  static constexpr size_t FieldsCount() { return Fields::Counter; }

  uint32_t m_stars = 0;
  uint32_t m_priceCategory = 0;
  double m_ratingBooking = 0.0;
  double m_ratingUser = 0.0;
  uint32_t m_type = 0;
  std::string m_translations;
  std::string m_descUrl;
};

using BookingDataset = SponsoredDataset<BookingHotel>;
}  // namespace generator
