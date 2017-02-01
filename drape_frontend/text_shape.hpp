#pragma once

#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "drape/constants.hpp"
#include "drape/glsl_types.hpp"

#include "geometry/point2d.hpp"

namespace df
{

class StraightTextLayout;

class TextShape : public MapShape
{
public:
  TextShape(m2::PointD const & basePoint, TextViewParams const & params,
            TileKey const & tileKey, bool hasPOI,
            uint32_t textIndex, bool affectedByZoomPriority,
            int displacementMode = dp::displacement::kAllModes,
            uint16_t specialModePriority = 0xFFFF);

  void Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const override;
  MapShapeType GetType() const override { return MapShapeType::OverlayType; }

  // Only for testing purposes!
  void DisableDisplacing() { m_disableDisplacing = true; }

private:
  void DrawSubString(StraightTextLayout const & layout, dp::FontDecl const & font,
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
  bool m_hasPOI;
  bool m_affectedByZoomPriority;
  uint32_t m_textIndex;

  bool m_disableDisplacing = false;
  int m_displacementMode;
  uint16_t m_specialModePriority;
};

} // namespace df
