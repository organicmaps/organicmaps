#include "base/SRC_FIRST.hpp"

#include "graphics/font_desc.hpp"

#include "std/cmath.hpp"
#include "std/algorithm.hpp"


namespace graphics
{
  FontDesc const & FontDesc::defaultFont = FontDesc();

  FontDesc::FontDesc(int size, graphics::Color const & color,
                     bool isMasked, graphics::Color const & maskColor)
                     : m_size(size), m_color(color),
                       m_isMasked(isMasked), m_maskColor(maskColor)
  {}

  bool FontDesc::IsValid() const
  {
    return m_size != -1;
  }

  bool FontDesc::operator ==(FontDesc const & src) const
  {
    return (m_size == src.m_size)
        && (m_color == src.m_color)
        && (m_isMasked == src.m_isMasked)
        && (m_maskColor == src.m_maskColor);
  }

  bool FontDesc::operator !=(FontDesc const & src) const
  {
    return !(*this == src);
  }

  bool FontDesc::operator < (FontDesc const & src) const
  {
    if (m_size != src.m_size)
      return m_size < src.m_size;
    if (m_color != src.m_color)
      return m_color < src.m_color;
    if (m_isMasked != src.m_isMasked)
      return m_isMasked < src.m_isMasked;
    if (m_maskColor != src.m_maskColor)
      return m_maskColor < src.m_maskColor;
    return false;
  }

  bool FontDesc::operator > (FontDesc const & src) const
  {
    if (m_size != src.m_size)
      return m_size > src.m_size;
    if (m_color != src.m_color)
      return m_color > src.m_color;
    if (m_isMasked != src.m_isMasked)
      return m_isMasked > src.m_isMasked;
    if (m_maskColor != src.m_maskColor)
      return m_maskColor > src.m_maskColor;
    return false;
  }
}
