#pragma once

#include "map_shape.hpp"
#include "shape_view_params.hpp"

#include "../geometry/point2d.hpp"

#include "../base/string_utils.hpp"

namespace df
{

class TextLayout;
class TextShape : public MapShape
{
public:
  TextShape(m2::PointF const & basePoint, TextViewParams const & params);

  void Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const;

private:
  void DrawSingleLine(dp::RefPointer<dp::Batcher> batcher, TextLayout const & layout) const;
  void DrawDoubleLine(dp::RefPointer<dp::Batcher> batcher, TextLayout const & primaryLayout,
                                                           TextLayout const & secondaryLayout) const;

private:
  m2::PointF m_basePoint;
  TextViewParams m_params;
};

} // namespace df
