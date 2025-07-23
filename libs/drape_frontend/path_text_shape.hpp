#pragma once

#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/path_text_handle.hpp"
#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/text_layout.hpp"

#include "geometry/spline.hpp"

#include <memory>
#include <vector>

namespace dp
{
class OverlayHandle;
}  // namespace dp

namespace df
{
class PathTextShape : public MapShape
{
public:
  PathTextShape(m2::SharedSpline const & spline, PathTextViewParams const & params, TileKey const & tileKey,
                uint32_t baseTextIndex);
  bool CalculateLayout(ref_ptr<dp::TextureManager> textures);

  std::vector<double> GetOffsets() const { return m_context->GetOffsets(); }

  void Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
            ref_ptr<dp::TextureManager> textures) const override;
  MapShapeType GetType() const override { return MapShapeType::OverlayType; }

private:
  uint64_t GetOverlayPriority(uint32_t textIndex, size_t textLength) const;

  void DrawPathTextPlain(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::TextureManager> textures,
                         ref_ptr<dp::Batcher> batcher) const;
  void DrawPathTextOutlined(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::TextureManager> textures,
                            ref_ptr<dp::Batcher> batcher) const;
  drape_ptr<dp::OverlayHandle> CreateOverlayHandle(uint32_t textIndex, ref_ptr<dp::TextureManager> textures) const;

  m2::SharedSpline m_spline;
  PathTextViewParams m_params;
  m2::PointI const m_tileCoords;
  uint32_t const m_baseTextIndex;
  std::shared_ptr<PathTextContext> m_context;
};
}  // namespace df
