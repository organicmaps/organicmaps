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
  void visSplit(strings::UniString const & visText,
                buffer_vector<strings::UniString, 3> & res,
                char const * delims,
                bool splitAllFound) const;
private:
  void DrawPolyLine(dp::RefPointer<dp::Batcher> batcher, vector<TextLayout> & layouts, vector<bool> & delims) const;

private:
  m2::PointF m_basePoint;
  TextViewParams m_params;
};

} // namespace df
