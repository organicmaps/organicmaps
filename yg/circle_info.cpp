#include "../base/SRC_FIRST.hpp"

#include "circle_info.hpp"

namespace yg
{
  CircleInfo::CircleInfo(unsigned radius,
                         Color const & color,
                         bool isOutlined,
                         Color const & outlineColor)
     : m_radius(radius),
     m_color(color),
     m_isOutlined(isOutlined),
     m_outlineColor(outlineColor)
  {}

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
    return l.m_outlineColor < r.m_outlineColor;
  }
}
