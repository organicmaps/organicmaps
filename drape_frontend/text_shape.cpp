#include "text_shape.hpp"
#include "common_structures.hpp"
#include "text_layout.hpp"
#include "fribidi.hpp"

#include "../drape/shader_def.hpp"
#include "../drape/attribute_provider.hpp"
#include "../drape/glstate.hpp"
#include "../drape/batcher.hpp"
#include "../drape/texture_set_holder.hpp"

#include "../base/math.hpp"
#include "../base/logging.hpp"
#include "../base/stl_add.hpp"
#include "../base/string_utils.hpp"

#include "../std/algorithm.hpp"
#include "../std/vector.hpp"

using m2::PointF;

namespace df
{

using m2::PointF;

namespace
{
  float const TEXT_EXPAND_FACTOR = 1.3f;

  PointF GetShift(dp::Anchor anchor, float width, float height)
  {
    switch(anchor)
    {
    case dp::Center:      return PointF(-width / 2.0f, height / 2.0f);
    case dp::Left:        return PointF(0.0f, height / 2.0f);
    case dp::Right:       return PointF(-width, height / 2.0f);
    case dp::Top:         return PointF(-width / 2.0f, height);
    case dp::Bottom:      return PointF(-width / 2.0f, 0);
    case dp::LeftTop:     return PointF(0.0f, height);
    case dp::RightTop:    return PointF(-width, height);
    case dp::LeftBottom:  return PointF(0.0f, 0.0f);
    case dp::RightBottom: return PointF(-width, 0.0f);
    default:              return PointF(0.0f, 0.0f);
    }
  }

  void BatchText(dp::RefPointer<dp::Batcher> batcher, int32_t textureSet,
                 vector<glsl_types::Quad4> const & positions,
                 vector<glsl_types::Quad4> const & texCoord,
                 vector<glsl_types::Quad4> const & fontColors,
                 vector<glsl_types::Quad4> const & outlineColor,
                 size_t glyphCount,
                 dp::OverlayHandle * handle)
  {
    ASSERT(glyphCount <= positions.size(), ());
    ASSERT(positions.size() == texCoord.size(), ());
    ASSERT(positions.size() == fontColors.size(), ());
    ASSERT(positions.size() == outlineColor.size(), ());

    dp::GLState state(gpu::FONT_PROGRAM, dp::GLState::OverlayLayer);
    state.SetTextureSet(textureSet);
    state.SetBlending(dp::Blending(true));

    dp::AttributeProvider provider(4, 4 * glyphCount);
    {
      dp::BindingInfo position(1);
      dp::BindingDecl & decl = position.GetBindingDecl(0);
      decl.m_attributeName = "a_position";
      decl.m_componentCount = 4;
      decl.m_componentType = gl_const::GLFloatType;
      decl.m_offset = 0;
      decl.m_stride = 0;
      provider.InitStream(0, position, dp::MakeStackRefPointer((void*)&positions[0]));
    }
    {
      dp::BindingInfo texcoord(1);
      dp::BindingDecl & decl = texcoord.GetBindingDecl(0);
      decl.m_attributeName = "a_texcoord";
      decl.m_componentCount = 4;
      decl.m_componentType = gl_const::GLFloatType;
      decl.m_offset = 0;
      decl.m_stride = 0;
      provider.InitStream(1, texcoord, dp::MakeStackRefPointer((void*)&texCoord[0]));
    }
    {
      dp::BindingInfo base_color(1);
      dp::BindingDecl & decl = base_color.GetBindingDecl(0);
      decl.m_attributeName = "a_color";
      decl.m_componentCount = 4;
      decl.m_componentType = gl_const::GLFloatType;
      decl.m_offset = 0;
      decl.m_stride = 0;
      provider.InitStream(2, base_color, dp::MakeStackRefPointer((void*)&fontColors[0]));
    }
    {
      dp::BindingInfo outline_color(1);
      dp::BindingDecl & decl = outline_color.GetBindingDecl(0);
      decl.m_attributeName = "a_outline_color";
      decl.m_componentCount = 4;
      decl.m_componentType = gl_const::GLFloatType;
      decl.m_offset = 0;
      decl.m_stride = 0;
      provider.InitStream(3, outline_color, dp::MakeStackRefPointer((void*)&outlineColor[0]));
    }

    batcher->InsertListOfStrip(state, dp::MakeStackRefPointer(&provider), MovePointer(handle), 4);
  }
}

TextShape::TextShape(m2::PointF const & basePoint, TextViewParams const & params)
    : m_basePoint(basePoint),
      m_params(params)
{
}

void TextShape::Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const
{
  ASSERT(!m_params.m_primaryText.empty(), ());
  if (m_params.m_secondaryText.empty())
  {
    df::FontDecl const & primaryFont = m_params.m_primaryTextFont;
    TextLayout const primaryLayout(fribidi::log2vis(strings::MakeUniString(m_params.m_primaryText)), primaryFont, textures);
    PointF const pixelOffset = GetShift(m_params.m_anchor, primaryLayout.GetPixelLength(), primaryFont.m_size) + m_params.m_primaryOffset;

    size_t glyphCount = primaryLayout.GetGlyphCount();
    vector<glsl_types::Quad4> positions(glyphCount);
    vector<glsl_types::Quad4> texCoord(glyphCount);
    vector<glsl_types::Quad4> fontColor(glyphCount);
    vector<glsl_types::Quad4> outlineColor(glyphCount);

    dp::OverlayHandle * handle = primaryLayout.LayoutText(m_params.m_featureID, m_basePoint,
                                                          pixelOffset, m_params.m_depth,
                                                          positions, texCoord, fontColor, outlineColor);
    BatchText(batcher, primaryLayout.GetTextureSet(),
              positions, texCoord,
              fontColor, outlineColor,
              glyphCount, handle);
  }
  else
  {
    df::FontDecl const & primaryFont = m_params.m_primaryTextFont;
    df::FontDecl const & secondaryFont = m_params.m_secondaryTextFont;

    TextLayout const primaryLayout(fribidi::log2vis(strings::MakeUniString(m_params.m_primaryText)), primaryFont, textures);
    TextLayout const secondaryLayout(fribidi::log2vis(strings::MakeUniString(m_params.m_secondaryText)), secondaryFont, textures);

    float const primaryTextLength = primaryLayout.GetPixelLength();
    float const secondaryTextLength = secondaryLayout.GetPixelLength();
    bool const isPrimaryLonger = primaryTextLength > secondaryTextLength;
    float const maxLength = max(primaryTextLength, secondaryTextLength);
    float const minLength = min(primaryTextLength, secondaryTextLength);
    float const halfLengthDiff = (maxLength - minLength) / 2.0;

    float const textHeight = TEXT_EXPAND_FACTOR * (primaryFont.m_size + secondaryFont.m_size);
    PointF const anchorOffset = GetShift(m_params.m_anchor, maxLength, textHeight);

    float const primaryDx = isPrimaryLonger ? 0.0f : halfLengthDiff;
    float const primaryDy = (1.0f - TEXT_EXPAND_FACTOR) * primaryFont.m_size - TEXT_EXPAND_FACTOR * secondaryFont.m_size;
    PointF const primaryPixelOffset = PointF(primaryDx, primaryDy) + anchorOffset + m_params.m_primaryOffset;

    size_t glyphCount = max(primaryLayout.GetGlyphCount(), secondaryLayout.GetGlyphCount());
    vector<glsl_types::Quad4> positions(glyphCount);
    vector<glsl_types::Quad4> texCoord(glyphCount);
    vector<glsl_types::Quad4> fontColor(glyphCount);
    vector<glsl_types::Quad4> outlineColor(glyphCount);

    {
      dp::OverlayHandle * handle = primaryLayout.LayoutText(m_params.m_featureID, m_basePoint,
                                                            primaryPixelOffset, m_params.m_depth,
                                                            positions, texCoord, fontColor, outlineColor);
      BatchText(batcher, primaryLayout.GetTextureSet(),
                positions, texCoord,
                fontColor, outlineColor,
                primaryLayout.GetGlyphCount(), handle);
    }

    float const secondaryDx = isPrimaryLonger ? halfLengthDiff : 0.0f;
    PointF const secondaryPixelOffset = PointF(secondaryDx, 0.0f) + anchorOffset + m_params.m_primaryOffset;

    {
      dp::OverlayHandle * handle = secondaryLayout.LayoutText(m_params.m_featureID, m_basePoint,
                                                              secondaryPixelOffset, m_params.m_depth,
                                                              positions, texCoord, fontColor, outlineColor);
      BatchText(batcher, secondaryLayout.GetTextureSet(),
                positions, texCoord,
                fontColor, outlineColor,
                secondaryLayout.GetGlyphCount(), handle);
    }
  }
}

} //end of df namespace
