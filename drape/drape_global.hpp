#pragma once

#include "color.hpp"

#include "base/assert.hpp"

namespace dp
{

enum TextureFormat
{
  RGBA8,
  ALPHA,
  UNSPECIFIED
};

uint8_t GetBytesPerPixel(TextureFormat format)
{
  uint8_t result = 0;
  switch (format)
  {
  case RGBA8:
    result = 4;
    break;
  case ALPHA:
    result = 1;
    break;
  default:
    ASSERT(false, ());
    break;
  }

  return result;
}

enum Anchor
{
  Center      = 0,
  Left        = 0x1,
  Right       = Left << 1,
  Top         = Right << 1,
  Bottom      = Top << 1,
  LeftTop     = Left | Top,
  RightTop    = Right | Top,
  LeftBottom  = Left | Bottom,
  RightBottom = Right | Bottom
};

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

struct FontDecl
{
  FontDecl() = default;
  FontDecl(Color const & color, float size, Color const & outlineColor = Color::Transparent())
    : m_color(color)
    , m_outlineColor(outlineColor)
    , m_size(size)
  {
  }

  Color m_color = Color::Transparent();
  Color m_outlineColor = Color::Transparent();
  float m_size = 0;
};

}
