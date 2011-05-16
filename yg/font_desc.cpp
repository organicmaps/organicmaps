#include "../base/SRC_FIRST.hpp"

#include "font_desc.hpp"

#include "../std/cmath.hpp"
#include "../std/algorithm.hpp"


namespace yg
{
  FontDesc const & FontDesc::defaultFont = FontDesc();

  FontDesc::FontDesc(bool isStatic, int size, yg::Color const & color,
                     bool isMasked, yg::Color const & maskColor)
                     : m_isStatic(isStatic), m_size(size), m_color(color),
                       m_isMasked(isMasked), m_maskColor(maskColor)
  {}

  void FontDesc::SetRank(uint8_t rank)
  {
    m_size += static_cast<int>(min(4.0E6, std::pow(1.1, double(rank))) / 2.0E6 * m_size);
    //m_size += static_cast<int>((double(rank) / 200.0) * m_size);
    //m_size += (rank / 20);
  }

  bool FontDesc::operator ==(FontDesc const & src) const
  {
    return (m_isStatic == src.m_isStatic)
        && (m_size == src.m_size)
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
    if (m_isStatic != src.m_isStatic)
      return m_isStatic < src.m_isStatic;
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
    if (m_isStatic != src.m_isStatic)
      return m_isStatic > src.m_isStatic;
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
