#pragma once

#include "storage/index.hpp"

#include "geometry/point2d.hpp"

#include <string>

namespace taxi
{
class Delegate
{
public:
  virtual ~Delegate() = default;

  virtual storage::TCountriesVec GetCountryIds(m2::PointD const & point) = 0;
  virtual std::string GetCityName(m2::PointD const & point) = 0;
  virtual storage::TCountryId GetMwmId(m2::PointD const & point) = 0;
};
}  // namespace taxi
