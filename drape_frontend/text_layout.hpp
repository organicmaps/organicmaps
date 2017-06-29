#pragma once

#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/intrusive_vector.hpp"

#include "drape/utils/vertex_decl.hpp"
#include "drape/glsl_types.hpp"
#include "drape/pointers.hpp"
#include "drape/texture_manager.hpp"

#include "geometry/spline.hpp"
#include "geometry/screenbase.hpp"

#include "base/string_utils.hpp"
#include "base/buffer_vector.hpp"

#include <memory>
#include <vector>

namespace dp
{
class OverlayHandle;
}  // namespace dp

namespace df
{
class TextLayout
{
public:
  virtual ~TextLayout() {}

  void Init(strings::UniString const & text,
            float fontSize, bool isSdf,
            ref_ptr<dp::TextureManager> textures);

  ref_ptr<dp::Texture> GetMaskTexture() const;
  uint32_t GetGlyphCount() const;
  float GetPixelLength() const;
  float GetPixelHeight() const;
  int GetFixedHeight() const { return m_fixedHeight; }
  strings::UniString const & GetText() const;

protected:
  using GlyphRegion = dp::TextureManager::GlyphRegion;

  dp::TextureManager::TGlyphsBuffer m_metrics;
  strings::UniString m_text;
  float m_textSizeRatio = 0.0f;
  int m_fixedHeight = dp::GlyphManager::kDynamicGlyphSize;
};

class StraightTextLayout : public TextLayout
{
  using TBase = TextLayout;
public:
  StraightTextLayout(strings::UniString const & text,
                     float fontSize, bool isSdf,
                     ref_ptr<dp::TextureManager> textures,
                     dp::Anchor anchor);

  void Cache(const glm::vec4 & pivot, glsl::vec2 const & pixelOffset,
             dp::TextureManager::ColorRegion const & colorRegion,
             dp::TextureManager::ColorRegion const & outlineRegion,
             gpu::TTextOutlinedStaticVertexBuffer & staticBuffer,
             gpu::TTextDynamicVertexBuffer & dynamicBuffer) const;

  void Cache(const glm::vec4 & pivot, glsl::vec2 const & pixelOffset,
             dp::TextureManager::ColorRegion const & color,
             gpu::TTextStaticVertexBuffer & staticBuffer,
             gpu::TTextDynamicVertexBuffer & dynamicBuffer) const;

  m2::PointF const & GetPixelSize() const { return m_pixelSize; }

private:
  buffer_vector<pair<size_t, glsl::vec2>, 2> m_offsets;
  m2::PointF m_pixelSize;
};

class PathTextLayout : public TextLayout
{
  using TBase = TextLayout;
public:
  PathTextLayout(m2::PointD const & tileCenter, strings::UniString const & text,
                 float fontSize, bool isSdf, ref_ptr<dp::TextureManager> textures);

  void CacheStaticGeometry(dp::TextureManager::ColorRegion const & colorRegion,
                           dp::TextureManager::ColorRegion const & outlineRegion,
                           gpu::TTextOutlinedStaticVertexBuffer & staticBuffer) const;

  void CacheStaticGeometry(dp::TextureManager::ColorRegion const & colorRegion,
                           gpu::TTextStaticVertexBuffer & staticBuffer) const;

  bool CacheDynamicGeometry(m2::Spline::iterator const & iter,
                            float depth,
                            m2::PointD const & globalPivot,
                            gpu::TTextDynamicVertexBuffer & buffer) const;

  static bool CalculatePerspectivePosition(float pixelSplineLength, float textPixelLength,
                                           uint32_t textIndex, float & offset);

  static void CalculatePositions(float splineLength, float splineScaleToPixel,
                                 float textPixelLength, std::vector<float> & offsets);
private:
  static float CalculateTextLength(float textPixelLength);

  m2::PointD m_tileCenter;
};

class SharedTextLayout
{
public:
  SharedTextLayout() = default;
  SharedTextLayout(PathTextLayout * layout);

  bool IsNull() const;
  void Reset(PathTextLayout * layout);
  PathTextLayout * GetRaw();

  PathTextLayout * operator->();
  PathTextLayout const * operator->() const;

private:
  std::shared_ptr<PathTextLayout> m_layout;
};
}  // namespace df
