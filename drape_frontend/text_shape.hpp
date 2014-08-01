#pragma once

#include "map_shape.hpp"
#include "shape_view_params.hpp"

#include "../geometry/point2d.hpp"

#include "../base/string_utils.hpp"

namespace df
{

namespace
{
  struct TextLine
  {
    m2::PointF m_offset;
    strings::UniString m_text;
    float m_length;
    FontDecl m_font;

    TextLine() {}
    TextLine(m2::PointF const & offset, strings::UniString const & text, float length, FontDecl const & font)
      : m_offset(offset), m_text(text), m_length(length), m_font(font) {}
  };
}

class TextShape : public MapShape
{
public:
  TextShape(m2::PointF const & basePoint, TextViewParams const & params);

  virtual void Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const;

private:
  void DrawTextLine(TextLine const & textLine, dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const;
  void DrawUnicalTextLine(TextLine const & textLine, int setNum, int letterCount,
                                dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const;

private:
  m2::PointF m_basePoint;
  TextViewParams m_params;
};

} // namespace df
