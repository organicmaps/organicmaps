#pragma once

#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "drape/constants.hpp"

namespace df
{

class ColoredSymbolShape : public MapShape
{
public:
  ColoredSymbolShape(m2::PointD const & mercatorPt, ColoredSymbolViewParams const & params,
                     TileKey const & tileKey, uint32_t textIndex, bool needOverlay = true,
                     int displacementMode = dp::displacement::kAllModes,
                     uint16_t specialModePriority = 0xFFFF);
  void Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const override;
  MapShapeType GetType() const override { return MapShapeType::OverlayType; }

private:
  uint64_t GetOverlayPriority() const;

  m2::PointD const m_point;
  ColoredSymbolViewParams m_params;
  m2::PointI const m_tileCoords;
  uint32_t const m_textIndex;
  bool const m_needOverlay;
  int const m_displacementMode;
  uint16_t const m_specialModePriority;
};

} //  namespace df
