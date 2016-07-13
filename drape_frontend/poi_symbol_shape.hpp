#pragma once

#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "drape/constants.hpp"

namespace df
{

class PoiSymbolShape : public MapShape
{
public:
  PoiSymbolShape(m2::PointF const & mercatorPt, PoiSymbolViewParams const & params,
                 int displacementMode = dp::displacement::kAllModes,
                 uint16_t specialModePriority = 0xFFFF);

  void Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const override;
  MapShapeType GetType() const override { return MapShapeType::OverlayType; }

private:
  uint64_t GetOverlayPriority() const;

  m2::PointF const m_pt;
  PoiSymbolViewParams const m_params;
  int const m_displacementMode;
  uint16_t const m_specialModePriority;
};

} // namespace df

