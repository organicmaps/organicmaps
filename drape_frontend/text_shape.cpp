#include "drape_frontend/text_shape.hpp"
#include "drape_frontend/text_handle.hpp"
#include "drape_frontend/text_layout.hpp"

#include "drape/utils/vertex_decl.hpp"
#include "drape/shader_def.hpp"
#include "drape/attribute_provider.hpp"
#include "drape/glstate.hpp"
#include "drape/batcher.hpp"
#include "drape/texture_manager.hpp"

#include "base/string_utils.hpp"

#include "std/vector.hpp"

namespace df
{

namespace
{

class StraightTextHandle : public TextHandle
{
public:
  StraightTextHandle(FeatureID const & id, dp::Anchor anchor, glsl::vec2 const & pivot,
                     glsl::vec2 const & pxSize, glsl::vec2 const & offset,
                     double priority, gpu::TTextDynamicVertexBuffer && normals)
    : TextHandle(id, anchor, priority, move(normals))
    , m_pivot(glsl::ToPoint(pivot))
    , m_offset(glsl::ToPoint(offset))
    , m_size(glsl::ToPoint(pxSize))
  {}

  m2::RectD GetPixelRect(ScreenBase const & screen) const override
  {
    m2::PointD pivot = screen.GtoP(m_pivot) + m_offset;
    double x = pivot.x;
    double y = pivot.y;
    if (m_anchor & dp::Left)
      x += m_size.x;
    else if (m_anchor & dp::Right)
      x -= m_size.x;
    else
    {
      float halfWidth = m_size.x / 2.0f;
      x += halfWidth;
      pivot.x -= halfWidth;
    }

    if (m_anchor & dp::Top)
      y += m_size.y;
    else if (m_anchor & dp::Bottom)
      y -= m_size.y;
    else
    {
      float halfHeight = m_size.y / 2.0f;
      y += halfHeight;
      pivot.y -= halfHeight;
    }

    return m2::RectD(min(x, pivot.x), min(y, pivot.y),
                     max(x, pivot.x), max(y, pivot.y));
  }

  void GetPixelShape(ScreenBase const & screen, Rects & rects) const override
  {
    rects.push_back(m2::RectF(GetPixelRect(screen)));
  }

private:
  m2::PointF m_pivot;
  m2::PointF m_offset;
  m2::PointF m_size;
};

} // namespace

TextShape::TextShape(m2::PointF const & basePoint, TextViewParams const & params)
  : m_basePoint(basePoint),
    m_params(params)
{
}

void TextShape::Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const
{
  ASSERT(!m_params.m_primaryText.empty(), ());
  StraightTextLayout primaryLayout(strings::MakeUniString(m_params.m_primaryText),
                                   m_params.m_primaryTextFont.m_size, textures, m_params.m_anchor);
  glsl::vec2 primaryOffset = glsl::ToVec2(m_params.m_primaryOffset);

  if (!m_params.m_secondaryText.empty())
  {
    StraightTextLayout secondaryLayout(strings::MakeUniString(m_params.m_secondaryText),
                                       m_params.m_secondaryTextFont.m_size, textures, m_params.m_anchor);

    glsl::vec2 secondaryOffset = primaryOffset;

    if (m_params.m_anchor & dp::Top)
      secondaryOffset += glsl::vec2(0.0, primaryLayout.GetPixelSize().y);
    else if (m_params.m_anchor & dp::Bottom)
      primaryOffset -= glsl::vec2(0.0, secondaryLayout.GetPixelSize().y);
    else
    {
      primaryOffset -= glsl::vec2(0.0, primaryLayout.GetPixelSize().y / 2.0f);
      secondaryOffset += glsl::vec2(0.0, secondaryLayout.GetPixelSize().y / 2.0f);
    }

    if (secondaryLayout.GetGlyphCount() > 0)
      DrawSubString(secondaryLayout, m_params.m_secondaryTextFont, secondaryOffset, batcher, textures);
  }

  if (primaryLayout.GetGlyphCount() > 0)
    DrawSubString(primaryLayout, m_params.m_primaryTextFont, primaryOffset, batcher, textures);
}

void TextShape::DrawSubString(StraightTextLayout const & layout,
                              dp::FontDecl const & font,
                              glsl::vec2 const & baseOffset,
                              ref_ptr<dp::Batcher> batcher,
                              ref_ptr<dp::TextureManager> textures) const
{
  gpu::TTextStaticVertexBuffer staticBuffer;
  gpu::TTextDynamicVertexBuffer dynamicBuffer;

  dp::TextureManager::ColorRegion color, outline;
  textures->GetColorRegion(font.m_color, color);
  textures->GetColorRegion(font.m_outlineColor, outline);

  layout.Cache(glsl::vec3(glsl::ToVec2(m_basePoint), m_params.m_depth),
               baseOffset,
               color, outline,
               staticBuffer,
               dynamicBuffer);

  dp::GLState state(gpu::TEXT_PROGRAM, dp::GLState::OverlayLayer);
  ASSERT(color.GetTexture() == outline.GetTexture(), ());
  state.SetColorTexture(color.GetTexture());
  state.SetMaskTexture(layout.GetMaskTexture());

  gpu::TTextDynamicVertexBuffer initialDynBuffer(dynamicBuffer.size());

  m2::PointU const & pixelSize = layout.GetPixelSize();
  drape_ptr<dp::OverlayHandle> handle = make_unique_dp<StraightTextHandle>(m_params.m_featureID,
                                                                           m_params.m_anchor,
                                                                           glsl::ToVec2(m_basePoint),
                                                                           glsl::vec2(pixelSize.x, pixelSize.y),
                                                                           baseOffset, m_params.m_depth,
                                                                           move(dynamicBuffer));

  dp::AttributeProvider provider(2, staticBuffer.size());
  provider.InitStream(0, gpu::TextStaticVertex::GetBindingInfo(), make_ref(staticBuffer.data()));
  provider.InitStream(1, gpu::TextDynamicVertex::GetBindingInfo(), make_ref(initialDynBuffer.data()));
  batcher->InsertListOfStrip(state, make_ref(&provider), move(handle), 4);
}

} //end of df namespace
