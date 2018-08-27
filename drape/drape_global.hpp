#pragma once

#include "drape/color.hpp"

#include "geometry/point2d.hpp"

#include "base/assert.hpp"

#include <cstdint>

namespace dp
{
enum class ApiVersion
{
  Invalid = -1,
  OpenGLES2 = 0,
  OpenGLES3,
  Metal
};

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

using DrapeID = uint64_t;

struct FontDecl
{
  FontDecl() = default;
  FontDecl(Color const & color, float size, bool isSdf = true,
           Color const & outlineColor = Color::Transparent())
    : m_color(color), m_outlineColor(outlineColor), m_size(size), m_isSdf(isSdf)
  {}

  Color m_color = Color::Transparent();
  Color m_outlineColor = Color::Transparent();
  float m_size = 0;
  bool m_isSdf = true;
};

struct TitleDecl
{
  dp::FontDecl m_primaryTextFont;
  std::string m_primaryText;
  dp::FontDecl m_secondaryTextFont;
  std::string m_secondaryText;
  dp::Anchor m_anchor = dp::Anchor::Center;
  m2::PointF m_primaryOffset = m2::PointF(0.0f, 0.0f);
  m2::PointF m_secondaryOffset = m2::PointF(0.0f, 0.0f);
  bool m_primaryOptional = false;
  bool m_secondaryOptional = false;
};

class BaseFramebuffer
{
public:
  virtual ~BaseFramebuffer() = default;
  virtual void Bind() = 0;
};

inline std::string DebugPrint(dp::ApiVersion apiVersion)
{
  switch (apiVersion)
  {
  case dp::ApiVersion::Invalid: return "Invalid";
  case dp::ApiVersion::OpenGLES2: return "OpenGLES2";
  case dp::ApiVersion::OpenGLES3: return "OpenGLES3";
  case dp::ApiVersion::Metal: return "Metal";
  }
  return "Unknown";
}
}  // namespace dp
