#pragma once

#include "geometry/latlon.hpp"

namespace topography_generator
{
template <typename ValueType>
class ValuesProvider
{
public:
  virtual ~ValuesProvider() = default;

  virtual ValueType GetValue(ms::LatLon const & pos) = 0;
  virtual ValueType GetInvalidValue() const = 0;
};
}  // namespace topography_generator
