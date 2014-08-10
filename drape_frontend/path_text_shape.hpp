#pragma once

#include "map_shape.hpp"
#include "shape_view_params.hpp"
#include "common_structures.hpp"

#include "../drape/overlay_handle.hpp"

#include "../std/vector.hpp"

#include "../geometry/point2d.hpp"
#include "../geometry/spline.hpp"

namespace df
{

class PathTextShape : public MapShape
{
public:
  PathTextShape(m2::SharedSpline const & spline,
                PathTextViewParams const & params,
                float const scaleGtoP);
  virtual void Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const;

private:
  m2::SharedSpline m_spline;
  PathTextViewParams m_params;
  float const m_scaleGtoP;
};

} // namespace df
