#pragma once

#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "geometry/spline.hpp"

namespace dp
{
class OverlayHandle;
}

namespace df
{
class PathSymbolShape : public MapShape
{
public:
  PathSymbolShape(m2::SharedSpline const & spline, PathSymbolViewParams const & params);
  void Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
            ref_ptr<dp::TextureManager> textures) const override;

  void Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures,
            TileKey const * trackTile) const;

private:
  PathSymbolViewParams m_params;
  m2::SharedSpline m_spline;
};

// Resolves arrows from all visible track tiles together at the current screen scale.
void PlaceTrackArrows(ScreenBase const & screen, std::vector<ref_ptr<dp::OverlayHandle>> & arrows);
}  // namespace df
