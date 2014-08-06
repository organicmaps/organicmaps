#include "text_shape.hpp"
#include "common_structures.hpp"
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
using glsl_types::vec2;
using glsl_types::vec3;
using glsl_types::vec4;

namespace
{
  static float const realFontSize = 28.0f;
  static float const fontOffset = 1.3f;

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

  class TextHandle : public dp::OverlayHandle
  {
  public:
    TextHandle(FeatureID const & id, m2::PointD const & pivot, m2::PointD const & pxSize,
               m2::PointD const & offset, double priority)
              : OverlayHandle(id, dp::LeftBottom, priority), m_pivot(pivot)
              , m_offset(offset), m_size(pxSize) {}

    m2::RectD GetPixelRect(ScreenBase const & screen) const
    {
      m2::PointD const pivot = screen.GtoP(m_pivot) + m_offset;
      return m2::RectD(pivot, pivot + m_size);
    }

  private:
    m2::PointD m_pivot;
    m2::PointD m_offset;
    m2::PointD m_size;
  };
}

TextShape::TextShape(m2::PointF const & basePoint, TextViewParams const & params)
    : m_basePoint(basePoint),
      m_params(params)
{
}

void TextShape::DrawTextLine(TextLine const & textLine, dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const
{
  int const size = textLine.m_text.size();
  int const maxTextureSetCount = textures->GetMaxTextureSet();
  buffer_vector<int, 16> sizes(maxTextureSetCount);

  for (int i = 0; i < size; i++)
  {
    dp::TextureSetHolder::GlyphRegion region;
    if (!textures->GetGlyphRegion(textLine.m_text[i], region))
      continue;
    ++sizes[region.GetTextureNode().m_textureSet];
  }

  for (int i = 0; i < maxTextureSetCount; ++i)
  {
    if (sizes[i])
      DrawUnicalTextLine(textLine, i, sizes[i], batcher, textures);
  }
}

void TextShape::DrawUnicalTextLine(TextLine const & textLine, int setNum, int letterCount,
                                   dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const
{
  strings::UniString text = textLine.m_text;
  float fontSize = textLine.m_font.m_size;
  float needOutline = textLine.m_font.m_needOutline;
  PointF anchorDelta = textLine.m_offset;

  int const numVert = letterCount * 4;
  vector<vec4> vertex(numVert);
  vec4 * vertexPt = &vertex[0];
  vector<vec4> texture(numVert);
  vec4 * texturePt = &texture[0];

  float stride = 0.0f;
  int textureSet;

  dp::TextureSetHolder::GlyphRegion region;
  for(int i = 0, j = 0 ; i < text.size() ; i++)
  {
    if (!textures->GetGlyphRegion(text[i], region))
      continue;
    float xOffset, yOffset, advance;
    region.GetMetrics(xOffset, yOffset, advance);
    float const aspect = fontSize / realFontSize;
    advance *= aspect;
    if (region.GetTextureNode().m_textureSet != setNum)
    {
      stride += advance;
      continue;
    }

    textureSet = region.GetTextureNode().m_textureSet;
    m2::RectF const rect = region.GetTexRect();
    float const textureNum = (region.GetTextureNode().m_textureOffset << 1) + needOutline;
    m2::PointU pixelSize;
    region.GetPixelSize(pixelSize);
    float const h = pixelSize.y * aspect;
    float const w = pixelSize.x * aspect;
    yOffset *= aspect;
    xOffset *= aspect;

    PointF const leftBottom(stride + xOffset + anchorDelta.x, yOffset + anchorDelta.y);
    PointF const rightBottom(stride + w + xOffset + anchorDelta.x, yOffset + anchorDelta.y);
    PointF const leftTop(stride + xOffset + anchorDelta.x, yOffset + h + anchorDelta.y);
    PointF const rightTop(stride + w + xOffset + anchorDelta.x, yOffset + h + anchorDelta.y);

    *vertexPt = vec4(m_basePoint, leftTop);
    vertexPt++;
    *vertexPt = vec4(m_basePoint, leftBottom);
    vertexPt++;
    *vertexPt = vec4(m_basePoint, rightTop);
    vertexPt++;
    *vertexPt = vec4(m_basePoint, rightBottom);
    vertexPt++;

    *texturePt = vec4(rect.minX(), rect.maxY(), textureNum, m_params.m_depth);
    texturePt++;
    *texturePt = vec4(rect.minX(), rect.minY(), textureNum, m_params.m_depth);
    texturePt++;
    *texturePt = vec4(rect.maxX(), rect.maxY(), textureNum, m_params.m_depth);
    texturePt++;
    *texturePt = vec4(rect.maxX(), rect.minY(), textureNum, m_params.m_depth);
    texturePt++;

    j++;
    stride += advance;
  }

  vector<vec4> color(numVert, vec4(textLine.m_font.m_color));
  vector<vec4> color2(numVert, vec4(textLine.m_font.m_outlineColor));

  dp::GLState state(gpu::FONT_PROGRAM, dp::GLState::OverlayLayer);
  state.SetTextureSet(textureSet);
  state.SetBlending(dp::Blending(true));

  dp::AttributeProvider provider(4, 4 * letterCount);
  {
    dp::BindingInfo position(1);
    dp::BindingDecl & decl = position.GetBindingDecl(0);
    decl.m_attributeName = "a_position";
    decl.m_componentCount = 4;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(0, position, dp::MakeStackRefPointer((void*)&vertex[0]));
  }
  {
    dp::BindingInfo texcoord(1);
    dp::BindingDecl & decl = texcoord.GetBindingDecl(0);
    decl.m_attributeName = "a_texcoord";
    decl.m_componentCount = 4;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(1, texcoord, dp::MakeStackRefPointer((void*)&texture[0]));
  }
  {
    dp::BindingInfo base_color(1);
    dp::BindingDecl & decl = base_color.GetBindingDecl(0);
    decl.m_attributeName = "a_color";
    decl.m_componentCount = 4;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(2, base_color, dp::MakeStackRefPointer((void*)&color[0]));
  }
  {
    dp::BindingInfo outline_color(1);
    dp::BindingDecl & decl = outline_color.GetBindingDecl(0);
    decl.m_attributeName = "a_outline_color";
    decl.m_componentCount = 4;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(3, outline_color, dp::MakeStackRefPointer((void*)&color2[0]));
  }

  PointF const dim = PointF(textLine.m_length, fontOffset * textLine.m_font.m_size);
  dp::OverlayHandle * handle = new TextHandle(m_params.m_featureID, m_basePoint, dim, anchorDelta, m_params.m_depth);
  batcher->InsertListOfStrip(state, dp::MakeStackRefPointer(&provider), MovePointer(handle), 4);
}

void TextShape::Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const
{
  strings::UniString const text = fribidi::log2vis(strings::MakeUniString(m_params.m_primaryText));
  int const size = text.size();
  float const fontSize = m_params.m_primaryTextFont.m_size;
  float textLength = 0.0f;
  vector<TextLine> lines(4);

  for (int i = 0 ; i < size ; i++)
  {
    dp::TextureSetHolder::GlyphRegion region;
    if (!textures->GetGlyphRegion(text[i], region))
    {
      LOG(LDEBUG, ("Symbol not found ", text[i]));
      continue;
    }
    float xOffset, yOffset, advance;
    region.GetMetrics(xOffset, yOffset, advance);
    textLength += advance * fontSize / realFontSize;
  }

  if (m_params.m_secondaryText.empty())
  {
    PointF const anchorDelta = GetShift(m_params.m_anchor, textLength, fontSize) + m_params.m_primaryOffset;
    lines[0] = TextLine(anchorDelta, text, textLength, m_params.m_primaryTextFont);
    DrawTextLine(lines[0], batcher, textures);
    return;
  }

  strings::UniString const auxText = fribidi::log2vis(strings::MakeUniString(m_params.m_secondaryText));
  int const auxSize = auxText.size();
  float const auxFontSize = m_params.m_secondaryTextFont.m_size;
  float auxTextLength = 0.0f;

  for (int i = 0 ; i < auxSize ; i++)
  {
    dp::TextureSetHolder::GlyphRegion region;
    if (!textures->GetGlyphRegion(auxText[i], region))
    {
      LOG(LDEBUG, ("Symbol not found(aux) ", auxText[i]));
      continue;
    }
    float xOffset, yOffset, advance;
    region.GetMetrics(xOffset, yOffset, advance);
    auxTextLength += advance * auxFontSize / realFontSize;
  }

  float const length = max(textLength, auxTextLength);
  PointF const anchorDelta = GetShift(m_params.m_anchor, length, fontOffset * fontSize + fontOffset * auxFontSize);
  float dx = textLength > auxTextLength ? 0.0f : (auxTextLength - textLength) / 2.0f;
  PointF const textDelta = PointF(dx, -fontOffset * auxFontSize + (1.0f - fontOffset) * fontSize) + anchorDelta + m_params.m_primaryOffset;
  dx = textLength > auxTextLength ? (textLength - auxTextLength) / 2.0f : 0.0f;
  PointF const auxTextDelta = PointF(dx, 0.0f) + anchorDelta + m_params.m_primaryOffset;

  lines[0] = TextLine(textDelta, text, textLength, m_params.m_primaryTextFont);
  lines[1] = TextLine(auxTextDelta, auxText, auxTextLength, m_params.m_secondaryTextFont);

  DrawTextLine(lines[0], batcher, textures);
  DrawTextLine(lines[1], batcher, textures);
}

} //end of df namespace
