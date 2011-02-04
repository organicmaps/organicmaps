#pragma once

#include "color.hpp"

namespace yg
{
  struct CircleInfo
  {
    double m_radius;
    Color m_color;
    bool m_isOutlined;
    Color m_outlineColor;

    CircleInfo(
        double radius,
        Color const & color = Color(0, 0, 0, 255),
        bool isOutlined = true,
        Color const & outlineColor = Color(255, 255, 255, 255));
  };
}
