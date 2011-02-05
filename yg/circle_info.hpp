#pragma once

#include "color.hpp"

namespace yg
{
  struct CircleInfo
  {
    unsigned m_radius;
    Color m_color;
    bool m_isOutlined;
    Color m_outlineColor;

    CircleInfo();
    CircleInfo(
        unsigned radius,
        Color const & color = Color(0, 0, 0, 255),
        bool isOutlined = false,
        Color const & outlineColor = Color(255, 255, 255, 255));
  };

  bool operator< (CircleInfo const & l, CircleInfo const & r);
}
