#pragma once

#include "drape_frontend/shape_view_params.hpp"

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

  void CacheStaticGeometry(dp::TextureManager::ColorRegion const & colorRegion,
                           gpu::TTextStaticVertexBuffer & staticBuffer) const;

  void CacheStaticGeometry(dp::TextureManager::ColorRegion const & colorRegion,
                           dp::TextureManager::ColorRegion const & outlineRegion,
                           gpu::TTextOutlinedStaticVertexBuffer & staticBuffer) const;

  void SetBasePosition(glm::vec4 const & pivot, glm::vec2 const & baseOffset);
  void CacheDynamicGeometry(glsl::vec2 const & pixelOffset, gpu::TTextDynamicVertexBuffer & dynamicBuffer) const;

  m2::PointF const & GetPixelSize() const { return m_pixelSize; }
  size_t GetRowsCount() const { return m_rowsCount; }

  static m2::PointF GetSymbolBasedTextOffset(m2::PointF const & symbolSize,
                                             dp::Anchor textAnchor,
                                             dp::Anchor symbolAnchor);

  glsl::vec2 GetTextOffset(m2::PointF const & symbolSize, dp::Anchor textAnchor, dp::Anchor symbolAnchor) const;

private:
  template <typename TGenerator>
  void Cache(TGenerator & generator) const
  {
    size_t beginOffset = 0;
    for (pair<size_t, glsl::vec2> const & node : m_offsets)
    {
      size_t const endOffset = node.first;
      generator.SetPenPosition(node.second);
      for (size_t index = beginOffset; index < endOffset && index < m_metrics.size(); ++index)
        generator(m_metrics[index]);
      beginOffset = endOffset;
    }
  }

  glm::vec2 m_baseOffset;
  glm::vec4 m_pivot;
  buffer_vector<pair<size_t, glsl::vec2>, 2> m_offsets;
  m2::PointF m_pixelSize;
  size_t m_rowsCount = 0;
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

  static void CalculatePositions(double splineLength, double splineScaleToPixel,
                                 double textPixelLength, std::vector<double> & offsets);
private:
  static double CalculateTextLength(double textPixelLength);

  m2::PointD m_tileCenter;
};

}  // namespace df
