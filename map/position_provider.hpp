#pragma once

#include "geometry/point2d.hpp"

#include <boost/optional.hpp>

class PositionProvider
{
public:
  virtual ~PositionProvider() = default;

  virtual boost::optional<m2::PointD> GetCurrentPosition() const = 0;
};
