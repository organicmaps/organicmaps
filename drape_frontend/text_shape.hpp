#pragma once

#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "geometry/point2d.hpp"
#include "drape/glsl_types.hpp"

namespace df
{

class StraightTextLayout;

class TextShape : public MapShape
{
public:
  TextShape(m2::PointF const & basePoint, TextViewParams const & params,
            bool hasPOI, size_t textIndex, bool affectedByZoomPriority);

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

  m2::PointF m_basePoint;
  TextViewParams m_params;
  bool m_hasPOI;
  bool m_affectedByZoomPriority;
  size_t m_textIndex;

  bool m_disableDisplacing = false;
};

} // namespace df
