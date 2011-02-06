#pragma once

#include "color.hpp"

namespace yg
{
  struct CircleInfo
  {
    unsigned m_radius;
    Color m_color;
    bool m_isOutlined;
    unsigned m_outlineWidth;
    Color m_outlineColor;

    CircleInfo();
    CircleInfo(
        double radius,
        Color const & color = Color(0, 0, 0, 255),
        bool isOutlined = false,
        double outlineWidth = 1,
        Color const & outlineColor = Color(255, 255, 255, 255));
  };

  bool operator< (CircleInfo const & l, CircleInfo const & r);
}
