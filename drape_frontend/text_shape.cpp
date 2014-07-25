#include "text_shape.hpp"

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

namespace
{
  static uint32_t const ListStride = 24;
  static float const realFontSize = 20.0f;

  void SetColor(vector<float> &dst, float const * ar, int index)
  {
    uint32_t const colorArraySize = 4;
    uint32_t const baseListIndex = ListStride * index;
    for (uint32_t i = 0; i < 6; ++i)
      memcpy(&dst[baseListIndex + colorArraySize * i], ar, colorArraySize * sizeof(float));
  }

  template <typename T>
  void QuadStripToList(vector<T> & dst, vector<T> & src, int32_t index)
  {
    static const int32_t dstStride = 6;
    static const int32_t srcStride = 4;
    const int32_t baseDstIndex = index * dstStride;
    const int32_t baseSrcIndex = index * srcStride;

    dst[baseDstIndex] = src[baseSrcIndex];
    dst[baseDstIndex + 1] = src[baseSrcIndex + 1];
    dst[baseDstIndex + 2] = src[baseSrcIndex + 2];
    dst[baseDstIndex + 3] = src[baseSrcIndex + 1];
    dst[baseDstIndex + 4] = src[baseSrcIndex + 3];
    dst[baseDstIndex + 5] = src[baseSrcIndex + 2];
  }

  PointF GetShift(dp::Anchor anchor, float width, float height)
  {
    switch(anchor)
    {
    case dp::Center:      return PointF(-width / 2.0f, -height / 2.0f);
    case dp::Left:        return PointF(0.0f, -height / 2.0f);
    case dp::Right:       return PointF(-width, -height / 2.0f);
    case dp::Top:         return PointF(-width / 2.0f, -height);
    case dp::Bottom:      return PointF(-width / 2.0f, 0);
    case dp::LeftTop:     return PointF(0.0f, -height);
    case dp::RightTop:    return PointF(-width, -height);
    case dp::LeftBottom:  return PointF(0.0f, 0.0f);
    case dp::RightBottom: return PointF(-width, 0.0f);
    default:              return PointF(0.0f, 0.0f);
    }
  }

  struct Vertex
  {
    Vertex() {}
    Vertex(m2::PointF const & pos, m2::PointF const & dir)
      : m_position(pos), m_direction(dir) {}

    Vertex(float posX, float posY, float dirX = 0.0f, float dirY = 0.0f)
      : m_position(posX, posY), m_direction(dirX, dirY) {}

    m2::PointF m_position;
    m2::PointF m_direction;
  };

  struct TexCoord
  {
    TexCoord() {}
    TexCoord(m2::PointF const & tex, float index = 0.0f, float depth = 0.0f)
      : m_texCoord(tex), m_index(index), m_depth(depth) {}

    TexCoord(float texX, float texY, float index = 0.0f, float depth = 0.0f)
      : m_texCoord(texX, texY), m_index(index), m_depth(depth) {}

    m2::PointF m_texCoord;
    float m_index;
    float m_depth;
  };
}

TextShape::TextShape(m2::PointF const & basePoint, TextViewParams const & params)
    : m_basePoint(basePoint),
      m_params(params)
{
}

void TextShape::AddGeometryWithTheSameTextureSet(int setNum, int letterCount, bool auxText,
                          float maxTextLength, PointF const & anchorDelta,
                          RefPointer<Batcher> batcher, RefPointer<TextureSetHolder> textures) const
{
  strings::UniString text;
  float fontSize;
  Color base, outline;
  float needOutline;
  if(auxText)
  {
    text = strings::MakeUniString(m_params.m_secondaryText);
    fontSize = m_params.m_secondaryTextFont.m_size;
    base = m_params.m_secondaryTextFont.m_color;
    outline = m_params.m_secondaryTextFont.m_outlineColor;
    needOutline = m_params.m_secondaryTextFont.m_needOutline;
  }
  else
  {
    text = strings::MakeUniString(m_params.m_primaryText);
    fontSize = m_params.m_primaryTextFont.m_size;
    base = m_params.m_primaryTextFont.m_color;
    outline = m_params.m_primaryTextFont.m_outlineColor;
    needOutline = m_params.m_primaryTextFont.m_needOutline;
  }

  int const numVert = letterCount * 4;
  vector<Vertex> vertex(numVert);
  vector<TexCoord> texture(numVert);

  float stride = 0.0f;
  int textureSet;

  TextureSetHolder::GlyphRegion region;
  for(int i = 0, j = 0 ; i < text.size() ; i++)
  {
    textures->GetGlyphRegion(text[i], region);
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

    int index = j * 4;
    vertex[index++] = Vertex(m_basePoint, leftTop);
    vertex[index++] = Vertex(m_basePoint, leftBottom);
    vertex[index++] = Vertex(m_basePoint, rightTop);
    vertex[index++] = Vertex(m_basePoint, rightBottom);

    index = j * 4;
    texture[index++] = TexCoord(rect.minX(), rect.maxY(), textureNum, m_params.m_depth);
    texture[index++] = TexCoord(rect.minX(), rect.minY(), textureNum, m_params.m_depth);
    texture[index++] = TexCoord(rect.maxX(), rect.maxY(), textureNum, m_params.m_depth);
    texture[index++] = TexCoord(rect.maxX(), rect.minY(), textureNum, m_params.m_depth);

    j++;
    stride += advance;
  }

  vector<Vertex> vertex2(numVert * 3 / 2);
  vector<TexCoord> texture2(numVert * 3 / 2);
  vector<float> color(numVert * 6);
  vector<float> color2(numVert * 6);

  float clr1[4], clr2[4];
  Convert(base, clr1[0], clr1[1], clr1[2], clr1[3]);
  Convert(outline, clr2[0], clr2[1], clr2[2], clr2[3]);

  for(int i = 0; i < letterCount ; i++)
  {
    QuadStripToList(vertex2, vertex, i);
    QuadStripToList(texture2, texture, i);
    SetColor(color, clr1, i);
    SetColor(color2, clr2, i);
  }

  GLState state(gpu::FONT_PROGRAM, GLState::OverlayLayer);
  state.SetTextureSet(textureSet);
  state.SetBlending(Blending(true));

  AttributeProvider provider(4, 6*letterCount);
  {
    BindingInfo position(1);
    BindingDecl & decl = position.GetBindingDecl(0);
    decl.m_attributeName = "a_position";
    decl.m_componentCount = 4;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(0, position, MakeStackRefPointer((void*)&vertex2[0]));
  }
  {
    BindingInfo texcoord(1);
    BindingDecl & decl = texcoord.GetBindingDecl(0);
    decl.m_attributeName = "a_texcoord";
    decl.m_componentCount = 4;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(1, texcoord, MakeStackRefPointer((void*)&texture2[0]));
  }
  {
    BindingInfo base_color(1);
    BindingDecl & decl = base_color.GetBindingDecl(0);
    decl.m_attributeName = "a_color";
    decl.m_componentCount = 4;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(2, base_color, MakeStackRefPointer((void*)&color[0]));
  }
  {
    BindingInfo outline_color(1);
    BindingDecl & decl = outline_color.GetBindingDecl(0);
    decl.m_attributeName = "a_outline_color";
    decl.m_componentCount = 4;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(3, outline_color, MakeStackRefPointer((void*)&color2[0]));
  }

  float const bbY = m_params.m_primaryTextFont.m_size + m_params.m_secondaryTextFont.m_size;
  OverlayHandle * handle = new SquareHandle(m_params.m_featureID,
                                           m_params.m_anchor,
                                           m_basePoint,
                                           m2::PointD(maxTextLength, bbY),
                                           m_params.m_depth);

  //handle->SetIsVisible(true);

  batcher->InsertTriangleList(state, MakeStackRefPointer(&provider), MovePointer(handle));
}

void TextShape::Draw(RefPointer<Batcher> batcher, RefPointer<TextureSetHolder> textures) const
{
  strings::UniString const text = strings::MakeUniString(m_params.m_primaryText);
  int const size = text.size();
  float const fontSize = m_params.m_primaryTextFont.m_size;
  float textLength = 0.0f;
  int const maxTextureSetCount = textures->GetMaxTextureSet();
  buffer_vector<int, 16> sizes(maxTextureSetCount);

  for (int i = 0 ; i < size ; i++)
  {
    TextureSetHolder::GlyphRegion region;
    textures->GetGlyphRegion(text[i], region);
    ++sizes[region.GetTextureNode().m_textureSet];
    float xOffset, yOffset, advance;
    region.GetMetrics(xOffset, yOffset, advance);
    textLength += advance * fontSize / realFontSize;
  }

  if (m_params.m_secondaryText.empty())
  {
    PointF const anchorDelta = GetShift(m_params.m_anchor, textLength, fontSize) + m_params.m_primaryOffset;
    for(int i = 0; i < maxTextureSetCount ; ++i)
    {
      if (sizes[i])
        AddGeometryWithTheSameTextureSet(i, sizes[i], false, textLength, anchorDelta, batcher, textures);
    }
    return;
  }

  strings::UniString const auxText = strings::MakeUniString(m_params.m_secondaryText);
  int const auxSize = auxText.size();
  float const auxFontSize = m_params.m_secondaryTextFont.m_size;
  float auxTextLength = 0.0f;
  buffer_vector<int, 16> auxSizes(maxTextureSetCount);

  for (int i = 0 ; i < auxSize ; i++)
  {
    TextureSetHolder::GlyphRegion region;
    textures->GetGlyphRegion(auxText[i], region);
    ++auxSizes[region.GetTextureNode().m_textureSet];
    float xOffset, yOffset, advance;
    region.GetMetrics(xOffset, yOffset, advance);
    auxTextLength += advance * auxFontSize / realFontSize;
  }

  float const length = max(textLength, auxTextLength);
  PointF const anchorDelta = GetShift(m_params.m_anchor, length, fontSize + auxFontSize);
  float dx = textLength > auxTextLength ? 0.0f : (auxTextLength - textLength) / 2.0f;
  PointF const textDelta = PointF(dx, auxFontSize) + anchorDelta + m_params.m_primaryOffset;
  dx = textLength > auxTextLength ? (textLength - auxTextLength) / 2.0f : 0.0f;
  PointF const auxTextDelta = PointF(dx, 0.0f) + anchorDelta + m_params.m_primaryOffset;

  for (int i = 0; i < maxTextureSetCount ; ++i)
  {
    if (sizes[i])
      AddGeometryWithTheSameTextureSet(i, sizes[i], false, length, textDelta, batcher, textures);
    if (auxSizes[i])
      AddGeometryWithTheSameTextureSet(i, auxSizes[i], true, length, auxTextDelta, batcher, textures);
  }
}

} //end of df namespace
