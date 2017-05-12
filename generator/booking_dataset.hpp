#pragma once

#include "generator/sponsored_dataset.hpp"

#include "geometry/latlon.hpp"

#include "base/newtype.hpp"

#include <limits>
#include <string>

namespace generator
{
// TODO(mgsergio): Try to get rid of code duplication. (See OpenTableRestaurant)
struct BookingHotel
{
  NEWTYPE(uint32_t, ObjectId);

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

  static constexpr ObjectId InvalidObjectId()
  {
    return ObjectId(std::numeric_limits<typename ObjectId::RepType>::max());
  }

  explicit BookingHotel(std::string const & src);

  static constexpr size_t FieldIndex(Fields field) { return static_cast<size_t>(field); }
  static constexpr size_t FieldsCount() { return static_cast<size_t>(Fields::Counter); }

  bool HasAddresParts() const { return !m_street.empty() || !m_houseNumber.empty(); }

  ObjectId m_id{InvalidObjectId()};
  ms::LatLon m_latLon = ms::LatLon::Zero();
  std::string m_name;
  std::string m_street;
  std::string m_houseNumber;

  std::string m_address;
  uint32_t m_stars = 0;
  uint32_t m_priceCategory = 0;
  double m_ratingBooking = 0.0;
  double m_ratingUser = 0.0;
  std::string m_descUrl;
  uint32_t m_type = 0;
  std::string m_translations;
};

ostream & operator<<(ostream & s, BookingHotel const & h);

NEWTYPE_SIMPLE_OUTPUT(BookingHotel::ObjectId);
using BookingDataset = SponsoredDataset<BookingHotel>;
}  // namespace generator
