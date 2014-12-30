#pragma once

#include "shape_view_params.hpp"
#include "intrusive_vector.hpp"

#include "../drape/utils/vertex_decl.hpp"
#include "../drape/glsl_types.hpp"
#include "../drape/pointers.hpp"
#include "../drape/texture_manager.hpp"

#include "../geometry/spline.hpp"
#include "../geometry/screenbase.hpp"

#include "../base/string_utils.hpp"
#include "../base/buffer_vector.hpp"

#include "../std/vector.hpp"
#include "../std/shared_ptr.hpp"

namespace dp
{
  class OverlayHandle;
}

namespace df
{

class TextLayout
{

public:
  enum class LayoutType
  {
    StraightLayout,
    PathLayout
  };

  dp::RefPointer<dp::Texture> GetMaskTexture() const;

  uint32_t GetGlyphCount() const;

  float GetPixelLength() const;
  float GetPixelHeight() const;
  LayoutType GetType() const { return m_type; }

protected:
  TextLayout(LayoutType type);
  void Init(strings::UniString const & text,
            float fontSize,
            dp::RefPointer<dp::TextureManager> textures);


protected:
  typedef dp::TextureManager::GlyphRegion GlyphRegion;

  dp::TextureManager::TGlyphsBuffer m_metrics;
  float m_textSizeRatio = 0.0;

private:
  LayoutType m_type;
};

class StraightTextLayout : public TextLayout
{
  typedef TextLayout TBase;
public:
  StraightTextLayout(strings::UniString const & text,
                     float fontSize,
                     dp::RefPointer<dp::TextureManager> textures,
                     dp::Anchor anchor);

  void Cache(glm::vec3 const & pivot, glsl::vec2 const & pixelOffset,
                   dp::TextureManager::ColorRegion const & colorRegion,
                   dp::TextureManager::ColorRegion const & outlineRegion,
                   gpu::TTextStaticVertexBuffer & staticBuffer,
                   gpu::TTextDynamicVertexBuffer & dynamicBuffer) const;

  m2::PointU const & GetPixelSize() const { return m_pixelSize; }

private:
  buffer_vector<pair<size_t, glsl::vec2>, 2> m_offsets;
  m2::PointU m_pixelSize;
};

class SharedTextLayout
{
public:
  SharedTextLayout(TextLayout * layout);

  bool IsNull() const;
  void Reset(TextLayout * layout);

  TextLayout * operator->();
  TextLayout const * operator->() const;

private:
  shared_ptr<TextLayout> m_layout;
};

}
