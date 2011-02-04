#include "../base/SRC_FIRST.hpp"

#include "circle_info.hpp"

namespace yg
{
  CircleInfo::CircleInfo(double radius,
                         Color const & color,
                         bool isOutlined,
                         Color const & outlineColor)
     : m_radius(radius),
     m_color(color),
     m_isOutlined(isOutlined),
     m_outlineColor(outlineColor)
  {}
}
