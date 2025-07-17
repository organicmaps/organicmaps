#pragma once

#include "geometry/point2d.hpp"

#include <optional>

class PositionProvider
{
public:
  virtual ~PositionProvider() = default;

  virtual std::optional<m2::PointD> GetCurrentPosition() const = 0;
};
