#pragma once

#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "drape/constants.hpp"
#include "drape/glsl_types.hpp"

#include "geometry/point2d.hpp"

#include <vector>

namespace df
{
class StraightTextLayout;

class TextShape : public MapShape
{
public:
  TextShape(m2::PointD const & basePoint, TextViewParams const & params,
            TileKey const & tileKey, m2::PointF const & symbolSize,
            dp::Anchor symbolAnchor, uint32_t textIndex);

  TextShape(m2::PointD const & basePoint, TextViewParams const & params,
            TileKey const & tileKey, std::vector<m2::PointF> const & symbolSizes,
            dp::Anchor symbolAnchor, uint32_t textIndex);

  void Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const override;
  MapShapeType GetType() const override { return MapShapeType::OverlayType; }

  // Only for testing purposes!
  void DisableDisplacing() { m_disableDisplacing = true; }

private:
  void DrawSubString(StraightTextLayout & layout, dp::FontDecl const & font,
                     glsl::vec2 const & baseOffset, ref_ptr<dp::Batcher> batcher,
                     ref_ptr<dp::TextureManager> textures, bool isPrimary, bool isOptional) const;

  void DrawSubStringPlain(StraightTextLayout const & layout, dp::FontDecl const & font,
                          glsl::vec2 const & baseOffset, ref_ptr<dp::Batcher> batcher,
                          ref_ptr<dp::TextureManager> textures, bool isPrimary, bool isOptional) const;
  void DrawSubStringOutlined(StraightTextLayout const & layout, dp::FontDecl const & font,
                             glsl::vec2 const & baseOffset, ref_ptr<dp::Batcher> batcher,
                             ref_ptr<dp::TextureManager> textures, bool isPrimary, bool isOptional) const;

  uint64_t GetOverlayPriority() const;

  m2::PointD m_basePoint;
  TextViewParams m_params;
  m2::PointI m_tileCoords;
  std::vector<m2::PointF> m_symbolSizes;
  dp::Anchor m_symbolAnchor;
  uint32_t m_textIndex;

  bool m_disableDisplacing = false;
};
}  // namespace df
