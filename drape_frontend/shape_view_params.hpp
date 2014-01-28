#pragma once

#include "../drape/color.hpp"

namespace df
{
  enum LineCap
  {
    RoundCap,
    ButtCap,
    SquareCap
  };

  enum LineJoin
  {
    NonJoin,
    RoundJoin,
    BevelJoin
  };

  struct LineViewParams
  {
    Color m_color;
    float m_width;
    LineCap m_cap;
    LineJoin m_join;
  };
}
