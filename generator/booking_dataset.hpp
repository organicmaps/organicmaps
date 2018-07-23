#pragma once

#include "generator/sponsored_dataset.hpp"
#include "generator/sponsored_object_base.hpp"

#include "geometry/latlon.hpp"

#include "base/newtype.hpp"

#include <limits>
#include <ostream>
#include <string>

namespace generator
{
struct BookingHotel : SponsoredObjectBase
{
  enum class Fields
  {
    Id = 0,
    Latitude = 1,
    Longtitude = 2,
    Name = 3,
    Address = 4,
    Stars = 5,
    PriceCategory = 6,
    RatingBooking = 7,
    RatingUsers = 8,
    DescUrl = 9,
    Type = 10,
    Translations = 11,
    Counter
  };

  explicit BookingHotel(std::string const & src);

  static constexpr size_t FieldIndex(Fields field) { return SponsoredObjectBase::FieldIndex(field); }
  static constexpr size_t FieldsCount() { return SponsoredObjectBase::FieldsCount<Fields>(); }

  uint32_t m_stars = 0;
  uint32_t m_priceCategory = 0;
  double m_ratingBooking = 0.0;
  double m_ratingUser = 0.0;
  uint32_t m_type = 0;
  std::string m_translations;
};

using BookingDataset = SponsoredDataset<BookingHotel>;
}  // namespace generator
