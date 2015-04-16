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

#include "std/vector.hpp"
#include "std/shared_ptr.hpp"

namespace dp
{
  class OverlayHandle;
}

namespace df
{

class TextLayout
{

public:
  virtual ~TextLayout() {}

  ref_ptr<dp::Texture> GetMaskTexture() const;

  uint32_t GetGlyphCount() const;

  float GetPixelLength() const;
  float GetPixelHeight() const;

protected:
  void Init(strings::UniString const & text,
            float fontSize,
            ref_ptr<dp::TextureManager> textures);

protected:
  typedef dp::TextureManager::GlyphRegion GlyphRegion;

  dp::TextureManager::TGlyphsBuffer m_metrics;
  float m_textSizeRatio = 0.0;
};

class StraightTextLayout : public TextLayout
{
  typedef TextLayout TBase;
public:
  StraightTextLayout(strings::UniString const & text,
                     float fontSize,
                     ref_ptr<dp::TextureManager> textures,
                     dp::Anchor anchor);

  void Cache(glsl::vec3 const & pivot, glsl::vec2 const & pixelOffset,
             dp::TextureManager::ColorRegion const & colorRegion,
             dp::TextureManager::ColorRegion const & outlineRegion,
             gpu::TTextStaticVertexBuffer & staticBuffer,
             gpu::TTextDynamicVertexBuffer & dynamicBuffer) const;

  m2::PointU const & GetPixelSize() const { return m_pixelSize; }

private:
  buffer_vector<pair<size_t, glsl::vec2>, 2> m_offsets;
  m2::PointU m_pixelSize;
};

class PathTextLayout : public TextLayout
{
  typedef TextLayout TBase;
public:
  PathTextLayout(strings::UniString const & text,
                 float fontSize, ref_ptr<dp::TextureManager> textures);

  void CacheStaticGeometry(glsl::vec3 const & pivot,
                           dp::TextureManager::ColorRegion const & colorRegion,
                           dp::TextureManager::ColorRegion const & outlineRegion,
                           gpu::TTextStaticVertexBuffer & staticBuffer) const;

  bool CacheDynamicGeometry(m2::Spline::iterator const & iter,
                            ScreenBase const & screen,
                            gpu::TTextDynamicVertexBuffer & buffer) const;
};

class SharedTextLayout
{
public:
  SharedTextLayout(PathTextLayout * layout);

  bool IsNull() const;
  void Reset(PathTextLayout * layout);
  PathTextLayout * GetRaw();

  PathTextLayout * operator->();
  PathTextLayout const * operator->() const;

private:
  shared_ptr<PathTextLayout> m_layout;
};

}
