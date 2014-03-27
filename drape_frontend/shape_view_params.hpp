#pragma once

#include "../drape/color.hpp"

#include "../std/string.hpp"

namespace df
{
  enum LineCap
  {
    SquareCap = -1,
    RoundCap  = 0,
    ButtCap   = 1,
  };

  enum LineJoin
  {
    MiterJoin   = -1,
    BevelJoin  = 0,
    RoundJoin = 1,
  };

  struct CommonViewParams
  {
    float m_depth;
  };

  struct PoiSymbolViewParams : CommonViewParams
  {
    string m_symbolName;
  };

  struct CircleViewParams : CommonViewParams
  {
    Color m_color;
    float m_radius;
  };

  struct AreaViewParams : CommonViewParams
  {
    Color m_color;
  };

  struct LineViewParams : CommonViewParams
  {
    Color m_color;
    float m_width;
    LineCap m_cap;
    LineJoin m_join;
  };
}
