#pragma once

#include "graphics/color.hpp"

namespace graphics
{
  struct FontDesc
  {
    int m_size;
    graphics::Color m_color;
    bool m_isMasked;
    graphics::Color m_maskColor;

    FontDesc(int size = -1, graphics::Color const & color = graphics::Color(0, 0, 0, 255),
             bool isMasked = false, graphics::Color const & maskColor = graphics::Color(255, 255, 255, 255));

    void SetRank(double rank);
    bool IsValid() const;

    bool operator != (FontDesc const & src) const;
    bool operator == (FontDesc const & src) const;
    bool operator < (FontDesc const & src) const;
    bool operator > (FontDesc const & src) const;

    static FontDesc const & defaultFont;
  };
}
