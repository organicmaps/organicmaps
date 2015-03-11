#include "gui_text.hpp"
#include "drape_gui.hpp"

#include "../base/string_utils.hpp"
#include "../base/stl_add.hpp"

#include "../drape/fribidi.hpp"

#include "../std/unique_ptr.hpp"
#include "../std/algorithm.hpp"

namespace gui
{

namespace
{
  static float const BASE_GLYPH_HEIGHT = 20.0f;

  glsl::vec2 GetNormalsAndMask(dp::TextureManager::GlyphRegion const & glyph, float textRatio,
                               array<glsl::vec2, 4> & normals, array<glsl::vec2, 4> & maskTexCoord)
  {
    m2::PointF pixelSize = m2::PointF(glyph.GetPixelSize()) * textRatio;
    m2::RectF const & r = glyph.GetTexRect();

    float xOffset = glyph.GetOffsetX() * textRatio;
    float yOffset = glyph.GetOffsetY() * textRatio;

    float const upVector = -static_cast<int32_t>(pixelSize.y) - yOffset;
    float const bottomVector = -yOffset;

    normals[0] = glsl::vec2(xOffset, bottomVector);
    normals[1] = glsl::vec2(xOffset, upVector);
    normals[2] = glsl::vec2(pixelSize.x + xOffset, bottomVector);
    normals[3] = glsl::vec2(pixelSize.x + xOffset, upVector);

    maskTexCoord[0] = glsl::ToVec2(r.LeftTop());
    maskTexCoord[1] = glsl::ToVec2(r.LeftBottom());
    maskTexCoord[2] = glsl::ToVec2(r.RightTop());
    maskTexCoord[3] = glsl::ToVec2(r.RightBottom());

    return glsl::vec2(xOffset, yOffset);
  }

  void FillPositionDecl(dp::BindingDecl & decl, uint8_t stride, uint8_t offset)
  {
    decl.m_attributeName = "a_position";
    decl.m_componentCount = 3;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_stride = stride;
    decl.m_offset = offset;
  }

  void FillNormalDecl(dp::BindingDecl & decl, uint8_t stride, uint8_t offset)
  {
    decl.m_attributeName = "a_normal";
    decl.m_componentCount = 2;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_stride = stride;
    decl.m_offset = offset;
  }

  void FillColorDecl(dp::BindingDecl & decl, uint8_t stride, uint8_t offset)
  {
    decl.m_attributeName = "a_colorTexCoord";
    decl.m_componentCount = 2;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_stride = stride;
    decl.m_offset = offset;
  }

  void FillOutlineDecl(dp::BindingDecl & decl, uint8_t stride, uint8_t offset)
  {
    decl.m_attributeName = "a_outlineColorTexCoord";
    decl.m_componentCount = 2;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_stride = stride;
    decl.m_offset = offset;
  }

  void FillMaskDecl(dp::BindingDecl & decl, uint8_t stride, uint8_t offset)
  {
    decl.m_attributeName = "a_maskTexCoord";
    decl.m_componentCount = 2;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_stride = stride;
    decl.m_offset = offset;
  }
}

dp::BindingInfo const & StaticLabel::Vertex::GetBindingInfo()
{
  static unique_ptr<dp::BindingInfo> info;

  if (info == nullptr)
  {
    info.reset(new dp::BindingInfo(5));
    uint8_t stride = sizeof(Vertex);
    uint8_t offset = 0;

    FillPositionDecl(info->GetBindingDecl(0), stride, offset);
    offset += sizeof(glsl::vec3);
    FillNormalDecl(info->GetBindingDecl(1), stride, offset);
    offset += sizeof(glsl::vec2);
    FillColorDecl(info->GetBindingDecl(2), stride, offset);
    offset += sizeof(glsl::vec2);
    FillOutlineDecl(info->GetBindingDecl(3), stride, offset);
    offset += sizeof(glsl::vec2);
    FillMaskDecl(info->GetBindingDecl(4), stride, offset);
    ASSERT_EQUAL(offset + sizeof(glsl::vec2), stride, ());
  }

  return *info.get();
}


void StaticLabel::CacheStaticText(string const & text, const char * delim,
                             dp::Anchor anchor, const dp::FontDecl & font,
                             dp::RefPointer<dp::TextureManager> mng, LabelResult & result)
{
  ASSERT(!text.empty(), ());

  dp::TextureManager::TMultilineText textParts;
  strings::Tokenize(text, delim, [&textParts](string const & part)
  {
    textParts.push_back(fribidi::log2vis(strings::MakeUniString(part)));
  });

  ASSERT(!textParts.empty(), ());

  dp::TextureManager::TMultilineGlyphsBuffer buffers;
  mng->GetGlyphRegions(textParts, buffers);

#ifdef DEBUG
  ASSERT_EQUAL(textParts.size(), buffers.size(), ());
  for (size_t i = 0; i < textParts.size(); ++i)
  {
    ASSERT(!textParts[i].empty(), ());
    ASSERT_EQUAL(textParts[i].size(), buffers[i].size(), ());
  }

  dp::RefPointer<dp::Texture> texture = buffers[0][0].GetTexture();
  for (dp::TextureManager::TGlyphsBuffer const & b : buffers)
  {
    for (dp::TextureManager::GlyphRegion const & reg : b)
      ASSERT(texture.GetRaw() == reg.GetTexture().GetRaw(), ());
  }
#endif

  dp::TextureManager::ColorRegion color;
  dp::TextureManager::ColorRegion outline;
  mng->GetColorRegion(font.m_color, color);
  mng->GetColorRegion(font.m_outlineColor, outline);
  ASSERT(color.GetTexture().GetRaw() == outline.GetTexture().GetRaw(), ());

  glsl::vec2 cTex = glsl::ToVec2(color.GetTexRect().Center());
  glsl::vec2 oTex = glsl::ToVec2(outline.GetTexRect().Center());

  float textRatio = font.m_size * DrapeGui::Instance().GetScaleFactor() / BASE_GLYPH_HEIGHT;

  buffer_vector<float, 3> lineLengths;
  lineLengths.resize(buffers.size());

  buffer_vector<size_t, 3> ranges;

  float fullHeight = 0.0;
  float prevLineHeight = 0.0;
  float firstLineHeight = 0.0;

  buffer_vector<Vertex, 128> & rb = result.m_buffer;

  for (size_t i = 0; i < buffers.size(); ++i)
  {
    dp::TextureManager::TGlyphsBuffer & regions = buffers[i];
    float & currentLineLength = lineLengths[i];

    float depth = 0.0;
    glsl::vec2 pen(0.0, prevLineHeight);
    prevLineHeight = 0.0;
    for (size_t j = 0; j < regions.size(); ++j)
    {
      array<glsl::vec2, 4> normals, maskTex;

      dp::TextureManager::GlyphRegion const & glyph = regions[j];
      glsl::vec2 offsets = GetNormalsAndMask(glyph, textRatio, normals, maskTex);

      glsl::vec3 position = glsl::vec3(0.0, 0.0, depth);

      for (size_t v = 0; v < normals.size(); ++v)
        rb.push_back(Vertex(position, pen + normals[v], cTex, oTex, maskTex[v]));

      float const advance = glyph.GetAdvanceX() * textRatio;
      currentLineLength += advance;
      prevLineHeight = max(prevLineHeight, offsets.y + glyph.GetPixelHeight() * textRatio);
      pen += glsl::vec2(advance, glyph.GetAdvanceY() * textRatio);

      depth += 10.0f;
    }

    ranges.push_back(rb.size());

    if (i == 0)
      firstLineHeight = prevLineHeight;

    fullHeight += prevLineHeight;
  }

  float const halfHeight = fullHeight / 2.0f;

  float yOffset = firstLineHeight - fullHeight / 2.0f;
  if (anchor & dp::Top)
    yOffset = firstLineHeight;
  else if (anchor & dp::Bottom)
    yOffset -= halfHeight;

  size_t startIndex = 0;
  for (size_t i = 0; i < ranges.size(); ++i)
  {
    float xOffset = -lineLengths[i] / 2.0f;
    if (anchor & dp::Left)
      xOffset = 0;
    else if (anchor & dp::Right)
      xOffset += xOffset;

    size_t endIndex = ranges[i];
    for (size_t i = startIndex; i < endIndex; ++i)
    {
      Vertex & v = rb[i];
      v.m_normal = v.m_normal + glsl::vec2(xOffset, yOffset);
    }

    startIndex = endIndex;
  }

  result.m_maskTexture = buffers[0][0].GetTexture();
  result.m_colorTexture = color.GetTexture();
}

dp::BindingInfo const & MutableLabel::StaticVertex::GetBindingInfo()
{
  static unique_ptr<dp::BindingInfo> info;

  if (info == nullptr)
  {
    info.reset(new dp::BindingInfo(3));

    uint8_t stride = sizeof(StaticVertex);
    uint8_t offset = 0;

    FillPositionDecl(info->GetBindingDecl(0), stride, offset);
    offset += sizeof(glsl::vec3);
    FillColorDecl(info->GetBindingDecl(1), stride, offset);
    offset += sizeof(glsl::vec2);
    FillOutlineDecl(info->GetBindingDecl(2), stride, offset);
    ASSERT_EQUAL(offset + sizeof(glsl::vec2), stride, ());
  }

  return *info.get();
}

dp::BindingInfo const & MutableLabel::DynamicVertex::GetBindingInfo()
{
  static unique_ptr<dp::BindingInfo> info;

  if (info == nullptr)
  {
    info.reset(new dp::BindingInfo(2, 1));
    uint8_t stride = sizeof(DynamicVertex);
    uint8_t offset = 0;

    FillNormalDecl(info->GetBindingDecl(0), stride, offset);
    offset += sizeof(glsl::vec2);
    FillMaskDecl(info->GetBindingDecl(1), stride, offset);
    ASSERT_EQUAL(offset + sizeof(glsl::vec2), stride, ());
  }

  return *info.get();
}

MutableLabel::MutableLabel(dp::Anchor anchor)
  : m_anchor(anchor)
{
}

void MutableLabel::SetMaxLength(uint16_t maxLength)
{
  m_maxLength = maxLength;
}

dp::RefPointer<dp::Texture> MutableLabel::SetAlphabet(string const & alphabet, dp::RefPointer<dp::TextureManager> mng)
{
  strings::UniString str = strings::MakeUniString(alphabet + ".");
  strings::UniString::iterator it = unique(str.begin(), str.end());
  str.resize(distance(str.begin(), it));

  dp::TextureManager::TGlyphsBuffer buffer;
  mng->GetGlyphRegions(str, buffer);
  m_alphabet.reserve(buffer.size());

  ASSERT_EQUAL(str.size(), buffer.size(), ());
  m_alphabet.resize(str.size());
  transform(str.begin(), str.end(), buffer.begin(), m_alphabet.begin(),
            [this](strings::UniChar const & c, dp::TextureManager::GlyphRegion const & r)
  {
    return make_pair(c, r);
  });

  sort(m_alphabet.begin(), m_alphabet.end(), [](TAlphabetNode const & n1, TAlphabetNode const & n2)
  {
    return n1.first < n2.first;
  });

  return m_alphabet[0].second.GetTexture();
}

dp::RefPointer<dp::Texture> MutableLabel::Precache(buffer_vector<StaticVertex, 128> & buffer,
                                                   dp::FontDecl const & font,
                                                   dp::RefPointer<dp::TextureManager> mng)
{
  m_textRatio = font.m_size * DrapeGui::Instance().GetScaleFactor() / BASE_GLYPH_HEIGHT;

  dp::TextureManager::ColorRegion color;
  dp::TextureManager::ColorRegion outlineColor;

  mng->GetColorRegion(font.m_color, color);
  mng->GetColorRegion(font.m_outlineColor, outlineColor);

  glsl::vec2 colorTex = glsl::ToVec2(color.GetTexRect().Center());
  glsl::vec2 outlineTex = glsl::ToVec2(outlineColor.GetTexRect().Center());

  size_t vertexCount = 4 * m_maxLength;
  buffer.resize(vertexCount, StaticVertex(glsl::vec3(0.0, 0.0, 0.0), colorTex, outlineTex));

  float depth = 0.0f;
  for (size_t i = 0; i < vertexCount; i += 4)
  {
    buffer[i + 0].m_position.z = depth;
    buffer[i + 1].m_position.z = depth;
    buffer[i + 2].m_position.z = depth;
    buffer[i + 3].m_position.z = depth;
    depth += 10.0f;
  }

  return color.GetTexture();
}

void MutableLabel::SetText(buffer_vector<DynamicVertex, 128> & buffer, string text) const
{
  if (text.size() > m_maxLength)
    text = text.erase(m_maxLength - 3) + "...";

  strings::UniString uniText = fribidi::log2vis(strings::MakeUniString(text));

  float maxHeight = 0.0f;
  float length = 0.0f;
  glsl::vec2 pen = glsl::vec2(0.0, 0.0);

  for (size_t i = 0; i < uniText.size(); ++i)
  {
    strings::UniChar c = uniText[i];
    TAlphabet::const_iterator it = find_if(m_alphabet.begin(), m_alphabet.end(), [&c](TAlphabetNode const & n)
    {
      return n.first == c;
    });

    ASSERT(it != m_alphabet.end(), ());
    if (it != m_alphabet.end())
    {
      array<glsl::vec2, 4> normals, maskTex;
      dp::TextureManager::GlyphRegion const & glyph = it->second;
      glsl::vec2 offsets = GetNormalsAndMask(glyph, m_textRatio, normals, maskTex);

      ASSERT_EQUAL(normals.size(), maskTex.size(), ());

      for (size_t i = 0; i < normals.size(); ++i)
        buffer.push_back(DynamicVertex(pen + normals[i], maskTex[i]));

      float const advance = glyph.GetAdvanceX() * m_textRatio;
      length += advance + offsets.x;
      pen += glsl::vec2(advance, glyph.GetAdvanceY() * m_textRatio);
      maxHeight = max(maxHeight, offsets.y  + glyph.GetPixelHeight() * m_textRatio);
    }
  }

  glsl::vec2 anchorModifyer = glsl::vec2(-length / 2.0f, maxHeight / 2.0f);
  if (m_anchor & dp::Right)
    anchorModifyer.x = -length;
  else if (m_anchor & dp::Left)
    anchorModifyer.x = 0;

  if (m_anchor & dp::Top)
    anchorModifyer.y = maxHeight;
  else if (m_anchor & dp::Bottom)
    anchorModifyer.y = 0;

  for (DynamicVertex & v : buffer)
    v.m_normal += anchorModifyer;

  for (size_t i = buffer.size(); i < 4 * m_maxLength; ++i)
    buffer.push_back(DynamicVertex(glsl::vec2(0.0, 0.0), glsl::vec2(0.0, 0.0)));
}

}
