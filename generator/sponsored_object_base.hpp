#pragma once

#include "geometry/latlon.hpp"

#include "base/newtype.hpp"

#include <iomanip>
#include <limits>
#include <string>

namespace generator
{
struct SponsoredObjectBase
{
  NEWTYPE(uint32_t, ObjectId);

  static constexpr ObjectId InvalidObjectId()
  {
    return ObjectId(std::numeric_limits<typename ObjectId::RepType>::max());
  }

  virtual ~SponsoredObjectBase() {}

  template<typename Fields>
  static constexpr size_t FieldIndex(Fields field) { return static_cast<size_t>(field); }

  template<typename Fields>
  static constexpr size_t FieldsCount() { return static_cast<size_t>(Fields::Counter); }

  bool HasAddresParts() const { return !m_street.empty() || !m_houseNumber.empty(); }

  ObjectId m_id{InvalidObjectId()};
  ms::LatLon m_latLon = ms::LatLon::Zero();
  std::string m_name;
  std::string m_street;
  std::string m_houseNumber;

  std::string m_address;
  std::string m_descUrl;
};

NEWTYPE_SIMPLE_OUTPUT(SponsoredObjectBase::ObjectId);

inline std::ostream & operator<<(std::ostream & s, SponsoredObjectBase const & h)
{
  s << std::fixed << std::setprecision(7);
  s << "Id: " << h.m_id << "\t Name: " << h.m_name << "\t Address: " << h.m_address
    << "\t lat: " << h.m_latLon.m_lat << " lon: " << h.m_latLon.m_lon;
  return s;
}
}  // namespace generator
