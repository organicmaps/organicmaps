#pragma once

#include "color.hpp"

#include "base/assert.hpp"

namespace dp
{
enum ApiVersion
{
  OpenGLES2 = 0,
  OpenGLES3
};

enum TextureFormat
{
  RGBA8,
  ALPHA,
  UNSPECIFIED
};

inline uint8_t GetBytesPerPixel(TextureFormat format)
{
  uint8_t result = 0;
  switch (format)
  {
  case RGBA8: result = 4; break;
  case ALPHA: result = 1; break;
  default: ASSERT(false, ()); break;
  }
  return result;
}

enum Anchor
{
  Center = 0,
  Left = 0x1,
  Right = Left << 1,
  Top = Right << 1,
  Bottom = Top << 1,
  LeftTop = Left | Top,
  RightTop = Right | Top,
  LeftBottom = Left | Bottom,
  RightBottom = Right | Bottom
};

enum LineCap
{
  SquareCap = -1,
  RoundCap = 0,
  ButtCap = 1,
};

enum LineJoin
{
  MiterJoin = -1,
  BevelJoin = 0,
  RoundJoin = 1,
};

struct FontDecl
{
  FontDecl() = default;
  FontDecl(Color const & color, float size, bool isSdf = true,
           Color const & outlineColor = Color::Transparent())
    : m_color(color), m_outlineColor(outlineColor), m_size(size), m_isSdf(isSdf)
  {
  }

  Color m_color = Color::Transparent();
  Color m_outlineColor = Color::Transparent();
  float m_size = 0;
  bool m_isSdf = true;
};

inline std::string DebugPrint(dp::ApiVersion apiVersion)
{
  if (apiVersion == dp::OpenGLES2)
    return "OpenGLES2";
  else if (apiVersion == dp::OpenGLES3)
    return "OpenGLES3";
  return "Unknown";
}
}  // namespace dp
