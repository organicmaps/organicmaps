#include "../base/SRC_FIRST.hpp"

#include "circle_info.hpp"

#include "../base/math.hpp"


namespace graphics
{
  CircleInfo::CircleInfo(double radius,
                         Color const & color,
                         bool isOutlined,
                         double outlineWidth,
                         Color const & outlineColor)
   : m_radius(my::rounds(radius)),
     m_color(color),
     m_isOutlined(isOutlined),
     m_outlineWidth(my::rounds(outlineWidth)),
     m_outlineColor(outlineColor)
  {
    if (!m_isOutlined)
    {
      m_outlineWidth = 0;
      m_outlineColor = graphics::Color(0, 0, 0, 0);
    }
  }

  CircleInfo::CircleInfo()
  {}

  bool operator< (CircleInfo const & l, CircleInfo const & r)
  {
    if (l.m_radius != r.m_radius)
      return l.m_radius < r.m_radius;
    if (l.m_color != r.m_color)
      return l.m_color < r.m_color;
    if (l.m_isOutlined != r.m_isOutlined)
      return l.m_isOutlined < r.m_isOutlined;
    if (l.m_outlineWidth != r.m_outlineWidth)
      return l.m_outlineWidth < r.m_outlineWidth;
    return l.m_outlineColor < r.m_outlineColor;
  }

  m2::PointU const CircleInfo::patternSize() const
  {
    unsigned r = m_isOutlined ? m_radius + m_outlineWidth : m_radius;
    return m2::PointU(r * 2 + 4, r * 2 + 4);
  }
}
