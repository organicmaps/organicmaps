#pragma once

#include "generator/sponsored_dataset.hpp"

#include "geometry/latlon.hpp"

#include "base/newtype.hpp"

#include "std/limits.hpp"
#include "std/string.hpp"

namespace generator
{
// TODO(mgsergio): Try to get rid of code duplication. (See BookingHotel)
struct OpentableRestaurant
{
  NEWTYPE(uint32_t, ObjectId);

  enum class Fields
  {
    Id = 0,
    Latitude,
    Longtitude,
    Name,
    Address,
    DescUrl,
    Phone,
    // Opentable doesn't have translations.
    // Translations,
    Counter
  };

  static constexpr ObjectId InvalidObjectId()
  {
    return ObjectId(numeric_limits<typename ObjectId::RepType>::max());
  }

  explicit OpentableRestaurant(string const & src);

  static constexpr size_t FieldIndex(Fields field) { return static_cast<size_t>(field); }
  static constexpr size_t FieldsCount() { return static_cast<size_t>(Fields::Counter); }

  bool HasAddresParts() const { return !m_street.empty() || !m_houseNumber.empty(); }

  ObjectId m_id{InvalidObjectId()};
  ms::LatLon m_latLon = ms::LatLon::Zero();
  string m_name;
  string m_street;
  string m_houseNumber;

  string m_address;
  string m_descUrl;
  // string m_translations;
};

ostream & operator<<(ostream & s, OpentableRestaurant const & r);

NEWTYPE_SIMPLE_OUTPUT(OpentableRestaurant::ObjectId);
using OpentableDataset = SponsoredDataset<OpentableRestaurant>;
}  // namespace generator
