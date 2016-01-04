#include "drape_frontend/text_shape.hpp"
#include "drape_frontend/text_handle.hpp"
#include "drape_frontend/text_layout.hpp"

#include "drape/utils/vertex_decl.hpp"
#include "drape/shader_def.hpp"
#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/glstate.hpp"
#include "drape/overlay_handle.hpp"
#include "drape/texture_manager.hpp"

#include "base/string_utils.hpp"

#include "std/vector.hpp"

namespace df
{

namespace
{

class StraightTextHandle : public TextHandle
{
  using TBase = TextHandle;

public:
  StraightTextHandle(FeatureID const & id, strings::UniString const & text,
                     dp::Anchor anchor, glsl::vec2 const & pivot,
                     glsl::vec2 const & pxSize, glsl::vec2 const & offset,
                     uint64_t priority, ref_ptr<dp::TextureManager> textureManager,
                     bool isOptional, gpu::TTextDynamicVertexBuffer && normals,
                     bool isBillboard = false)
    : TextHandle(id, text, anchor, priority, textureManager, move(normals), isBillboard)
    , m_pivot(glsl::ToPoint(pivot))
    , m_offset(glsl::ToPoint(offset))
    , m_size(glsl::ToPoint(pxSize))
    , m_isOptional(isOptional)
  {}

  m2::PointD GetPivot(ScreenBase const & screen, bool perspective) const override
  {
    m2::PointD pivot = TBase::GetPivot(screen, false);
    if (perspective)
      pivot = screen.PtoP3d(pivot - m_offset, -m_pivotZ / screen.GetScale()) + m_offset;
    return pivot;
  }

  m2::RectD GetPixelRect(ScreenBase const & screen, bool perspective) const override
  {
    if (perspective)
    {
      if (IsBillboard())
      {
        m2::PointD const pxPivot = screen.GtoP(m_pivot);
        m2::PointD const pxPivotPerspective = screen.PtoP3d(pxPivot, -m_pivotZ / screen.GetScale());

        m2::RectD pxRectPerspective = GetPixelRect(screen, false);
        pxRectPerspective.Offset(-pxPivot);
        pxRectPerspective.Offset(pxPivotPerspective);

        return pxRectPerspective;
      }
      return GetPixelRectPerspective(screen);
    }

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

  void GetPixelShape(ScreenBase const & screen, Rects & rects, bool perspective) const override
  {
    rects.emplace_back(GetPixelRect(screen, perspective));
  }

  bool IsBound() const override
  {
    return !m_isOptional;
  }

private:
  m2::PointF m_pivot;
  m2::PointF m_offset;
  m2::PointF m_size;
  bool m_isOptional;
};

} // namespace

TextShape::TextShape(m2::PointF const & basePoint, TextViewParams const & params, bool hasPOI)
  : m_basePoint(basePoint),
    m_params(params),
    m_hasPOI(hasPOI)
{}

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
      DrawSubString(secondaryLayout, m_params.m_secondaryTextFont, secondaryOffset, batcher,
                    textures, false /* isPrimary */, m_params.m_secondaryOptional);
  }

  if (primaryLayout.GetGlyphCount() > 0)
    DrawSubString(primaryLayout, m_params.m_primaryTextFont, primaryOffset, batcher,
                  textures, true /* isPrimary */, m_params.m_primaryOptional);
}

void TextShape::DrawSubString(StraightTextLayout const & layout, dp::FontDecl const & font,
                              glm::vec2 const & baseOffset, ref_ptr<dp::Batcher> batcher,
                              ref_ptr<dp::TextureManager> textures,
                              bool isPrimary, bool isOptional) const
{
  dp::Color outlineColor = isPrimary ? m_params.m_primaryTextFont.m_outlineColor
                                     : m_params.m_secondaryTextFont.m_outlineColor;

  if (outlineColor == dp::Color::Transparent())
  {
    DrawSubStringPlain(layout, font, baseOffset,batcher, textures, isPrimary, isOptional);
  }
  else
  {
    DrawSubStringOutlined(layout, font, baseOffset, batcher, textures, isPrimary, isOptional);
  }
}

void TextShape::DrawSubStringPlain(StraightTextLayout const & layout, dp::FontDecl const & font,
                                   glm::vec2 const & baseOffset, ref_ptr<dp::Batcher> batcher,
                                   ref_ptr<dp::TextureManager> textures, bool isPrimary, bool isOptional) const
{
  gpu::TTextStaticVertexBuffer staticBuffer;
  gpu::TTextDynamicVertexBuffer dynamicBuffer;

  dp::TextureManager::ColorRegion color, outline;
  textures->GetColorRegion(font.m_color, color);
  textures->GetColorRegion(font.m_outlineColor, outline);

  layout.Cache(glsl::vec4(glsl::ToVec2(m_basePoint), m_params.m_depth, -m_params.m_posZ),
               baseOffset, color, staticBuffer, dynamicBuffer);

  dp::GLState state(gpu::TEXT_PROGRAM, dp::GLState::OverlayLayer);
  state.SetProgram3dIndex(gpu::TEXT_BILLBOARD_PROGRAM);
  ASSERT(color.GetTexture() == outline.GetTexture(), ());
  state.SetColorTexture(color.GetTexture());
  state.SetMaskTexture(layout.GetMaskTexture());

  gpu::TTextDynamicVertexBuffer initialDynBuffer(dynamicBuffer.size());

  m2::PointU const & pixelSize = layout.GetPixelSize();

  drape_ptr<dp::OverlayHandle> handle = make_unique_dp<StraightTextHandle>(m_params.m_featureID,
                                                                           layout.GetText(),
                                                                           m_params.m_anchor,
                                                                           glsl::ToVec2(m_basePoint),
                                                                           glsl::vec2(pixelSize.x, pixelSize.y),
                                                                           baseOffset,
                                                                           GetOverlayPriority(),
                                                                           textures,
                                                                           isOptional,
                                                                           move(dynamicBuffer),
                                                                           true);
  handle->SetPivotZ(m_params.m_posZ);
  handle->SetOverlayRank(m_hasPOI ? (isPrimary ? dp::OverlayRank1 : dp::OverlayRank2) : dp::OverlayRank0);
  handle->SetExtendingSize(m_params.m_extendingSize);

  dp::AttributeProvider provider(2, staticBuffer.size());
  provider.InitStream(0, gpu::TextStaticVertex::GetBindingInfo(), make_ref(staticBuffer.data()));
  provider.InitStream(1, gpu::TextDynamicVertex::GetBindingInfo(), make_ref(initialDynBuffer.data()));
  batcher->InsertListOfStrip(state, make_ref(&provider), move(handle), 4);
}

void TextShape::DrawSubStringOutlined(StraightTextLayout const & layout, dp::FontDecl const & font,
                                      glm::vec2 const & baseOffset, ref_ptr<dp::Batcher> batcher,
                                      ref_ptr<dp::TextureManager> textures, bool isPrimary, bool isOptional) const
{
  gpu::TTextOutlinedStaticVertexBuffer staticBuffer;
  gpu::TTextDynamicVertexBuffer dynamicBuffer;

  dp::TextureManager::ColorRegion color, outline;
  textures->GetColorRegion(font.m_color, color);
  textures->GetColorRegion(font.m_outlineColor, outline);

  layout.Cache(glsl::vec4(glsl::ToVec2(m_basePoint), m_params.m_depth, -m_params.m_posZ),
               baseOffset, color, outline, staticBuffer, dynamicBuffer);

  dp::GLState state(gpu::TEXT_OUTLINED_PROGRAM, dp::GLState::OverlayLayer);
  state.SetProgram3dIndex(gpu::TEXT_OUTLINED_BILLBOARD_PROGRAM);
  ASSERT(color.GetTexture() == outline.GetTexture(), ());
  state.SetColorTexture(color.GetTexture());
  state.SetMaskTexture(layout.GetMaskTexture());

  gpu::TTextDynamicVertexBuffer initialDynBuffer(dynamicBuffer.size());

  m2::PointU const & pixelSize = layout.GetPixelSize();

  drape_ptr<dp::OverlayHandle> handle = make_unique_dp<StraightTextHandle>(m_params.m_featureID,
                                                                           layout.GetText(),
                                                                           m_params.m_anchor,
                                                                           glsl::ToVec2(m_basePoint),
                                                                           glsl::vec2(pixelSize.x, pixelSize.y),
                                                                           baseOffset,
                                                                           GetOverlayPriority(),
                                                                           textures,
                                                                           isOptional,
                                                                           move(dynamicBuffer),
                                                                           true);
  handle->SetPivotZ(m_params.m_posZ);
  handle->SetOverlayRank(m_hasPOI ? (isPrimary ? dp::OverlayRank1 : dp::OverlayRank2) : dp::OverlayRank0);
  handle->SetExtendingSize(m_params.m_extendingSize);

  dp::AttributeProvider provider(2, staticBuffer.size());
  provider.InitStream(0, gpu::TextOutlinedStaticVertex::GetBindingInfo(), make_ref(staticBuffer.data()));
  provider.InitStream(1, gpu::TextDynamicVertex::GetBindingInfo(), make_ref(initialDynBuffer.data()));
  batcher->InsertListOfStrip(state, make_ref(&provider), move(handle), 4);
}


uint64_t TextShape::GetOverlayPriority() const
{
  // Overlay priority for text shapes considers the existance of secondary string and length of primary text.
  // - If the text has secondary string then it has more priority;
  // - The more text length, the more priority.
  // [6 bytes - standard overlay priority][1 byte - secondary text][1 byte - length].
  static uint64_t constexpr kMask = ~static_cast<uint64_t>(0xFFFF);
  uint64_t priority = dp::CalculateOverlayPriority(m_params.m_minVisibleScale, m_params.m_rank, m_params.m_depth);
  priority &= kMask;
  if (!m_params.m_secondaryText.empty())
    priority |= 0xFF00;
  priority |= (0xFF - static_cast<uint8_t>(m_params.m_primaryText.size()));

  return priority;
}

} //end of df namespace
