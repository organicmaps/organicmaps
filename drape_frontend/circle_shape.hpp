#pragma once

#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"

namespace dp
{
class OverlayHandle;
} // namespace dp

namespace df
{

class CircleShape : public MapShape
{
public:
  CircleShape(m2::PointF const & mercatorPt, CircleViewParams const & params, bool needOverlay);

  void Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const override;
  MapShapeType GetType() const override { return MapShapeType::OverlayType; }

private:
  uint64_t GetOverlayPriority() const;
  drape_ptr<dp::OverlayHandle> CreateOverlay() const;

  m2::PointF m_pt;
  CircleViewParams m_params;
  bool m_needOverlay;
};

} //namespace df
