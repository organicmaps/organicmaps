#pragma once

#include "color.hpp"

#include "../std/vector.hpp"
#include "../base/buffer_vector.hpp"

namespace yg
{
  /// definition of the line style pattern used as a texture-cache-key
  struct PenInfo
  {
    //typedef buffer_vector<double, 8> TPattern;
    typedef vector<double> TPattern;
    Color m_color;
    double m_w;
    TPattern m_pat;
    double m_offset;

    bool m_isSolid;

    PenInfo(
        Color const & color,
        double width,
        double const * pattern,
        size_t patternSize,
        double offset);

    PenInfo();

    double firstDashOffset() const;
    bool atDashOffset(double offset) const;
  };

  bool operator < (PenInfo const & l, PenInfo const & r);
}
