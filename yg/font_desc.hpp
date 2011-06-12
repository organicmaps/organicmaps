#pragma once

#include "color.hpp"

namespace yg
{
  struct FontDesc
  {
    int m_size;
    yg::Color m_color;
    bool m_isMasked;
    yg::Color m_maskColor;

    FontDesc(int size = 10, yg::Color const & color = yg::Color(0, 0, 0, 255),
             bool isMasked = true, yg::Color const & maskColor = yg::Color(255, 255, 255, 255));

    void SetRank(double rank);

    bool operator != (FontDesc const & src) const;
    bool operator == (FontDesc const & src) const;
    bool operator < (FontDesc const & src) const;
    bool operator > (FontDesc const & src) const;

    static FontDesc const & defaultFont;
  };
}
