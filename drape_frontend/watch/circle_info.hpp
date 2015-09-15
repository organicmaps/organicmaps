#pragma once

#include "drape/drape_global.hpp"

namespace df
{
namespace watch
{

struct CircleInfo
{
  unsigned m_radius;
  dp::Color m_color;
  bool m_isOutlined;
  unsigned m_outlineWidth;
  dp::Color m_outlineColor;

  CircleInfo() = default;
  CircleInfo(double radius,
             dp::Color const & color = dp::Color(0, 0, 0, 255),
             bool isOutlined = false,
             double outlineWidth = 1,
             dp::Color const & outlineColor = dp::Color(255, 255, 255, 255))
    : m_radius(my::rounds(radius))
    , m_color(color)
    , m_isOutlined(isOutlined)
    , m_outlineWidth(my::rounds(outlineWidth))
    , m_outlineColor(outlineColor)
  {
    if (!m_isOutlined)
    {
      m_outlineWidth = 0;
      m_outlineColor = dp::Color(0, 0, 0, 0);
    }
  }
};

}
}
