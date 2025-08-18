#pragma once

#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "drape/constants.hpp"

namespace dp
{
class OverlayHandle;
}  // namespace dp

namespace df
{
class PoiSymbolShape : public MapShape
{
public:
  PoiSymbolShape(m2::PointD const & mercatorPt, PoiSymbolViewParams const & params, TileKey const & tileKey,
                 uint32_t textIndex);

  void Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
            ref_ptr<dp::TextureManager> textures) const override;
  MapShapeType GetType() const override { return MapShapeType::OverlayType; }

private:
  uint64_t GetOverlayPriority() const;
  drape_ptr<dp::OverlayHandle> CreateOverlayHandle(m2::RectD const & pixelRect) const;

  m2::PointD const m_pt;
  PoiSymbolViewParams const m_params;
  m2::PointI const m_tileCoords;
  uint32_t const m_textIndex;
};
}  // namespace df
