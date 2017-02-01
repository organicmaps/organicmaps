#pragma once

#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "geometry/spline.hpp"

namespace df
{

class PathTextLayout;

class PathTextShape : public MapShape
{
public:
  PathTextShape(m2::SharedSpline const & spline, PathTextViewParams const & params,
                TileKey const & tileKey, uint32_t baseTextIndex);
  void Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const override;
  MapShapeType GetType() const override { return MapShapeType::OverlayType; }

private:
  uint64_t GetOverlayPriority(uint32_t textIndex, bool followingMode) const;

  void DrawPathTextPlain(ref_ptr<dp::TextureManager> textures,
                         ref_ptr<dp::Batcher> batcher,
                         unique_ptr<PathTextLayout> && layout,
                         vector<float> const & offests) const;
  void DrawPathTextOutlined(ref_ptr<dp::TextureManager> textures,
                            ref_ptr<dp::Batcher> batcher,
                            unique_ptr<PathTextLayout> && layout,
                            vector<float> const & offsets) const;

  m2::SharedSpline m_spline;
  PathTextViewParams m_params;
  m2::PointI const m_tileCoords;
  uint32_t const m_baseTextIndex;
};

} // namespace df
