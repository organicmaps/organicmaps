#pragma once

#include "../geometry/point2d.hpp"
#include "../geometry/any_rect2d.hpp"

namespace srv
{
  class Viewport
  {
  public:
    Viewport(const m2::PointD & center, double scale, int width, int height);

    const m2::PointD & GetCenter() const;
    double GetScale() const;

    int GetWidth() const;
    int GetHeight() const;

    m2::AnyRectD GetAnyRect() const;

  private:
    m2::PointD m_center;
    double m_scale;
    int m_width;
    int m_height;
  };
}
