#pragma once

#include "map_shape.hpp"
#include "shape_view_params.hpp"

#include "../geometry/point2d.hpp"

namespace df
{
class TextShape : public MapShape
{
public:
  TextShape(m2::PointF const & basePoint, TextViewParams const & params);

  virtual void Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const;

private:
  void AddGeometryWithTheSameTextureSet(int setNum, int letterCount, bool auxText,
                                        float maxTextLength, m2::PointF const & anchorDelta,
                                        dp::RefPointer<dp::Batcher> batcher,
                                        dp::RefPointer<dp::TextureSetHolder> textures) const;

private:
  m2::PointF m_basePoint;
  TextViewParams m_params;
};

} // namespace df
