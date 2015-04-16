#pragma once

#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "geometry/point2d.hpp"
#include "drape/glsl_types.hpp"

namespace df
{

class StraightTextLayout;

class TextShape : public MapShape
{
public:
  TextShape(m2::PointF const & basePoint, TextViewParams const & params);

  void Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const;
private:
  void DrawSubString(StraightTextLayout const & layout, dp::FontDecl const & font,
                     glsl::vec2 const & baseOffset, ref_ptr<dp::Batcher> batcher,
                     ref_ptr<dp::TextureManager> textures) const;

private:
  m2::PointF m_basePoint;
  TextViewParams m_params;
};

} // namespace df
