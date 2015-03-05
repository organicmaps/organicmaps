#include "gui_text.hpp"
#include "drape_gui.hpp"

#include "../base/string_utils.hpp"
#include "../base/stl_add.hpp"

#include "../drape/fribidi.hpp"

#include "../std/unique_ptr.hpp"
#include "../std/algorithm.hpp"

namespace gui
{

dp::BindingInfo const & GuiText::StaticVertex::GetBindingInfo()
{
  static unique_ptr<dp::BindingInfo> info;

  if (info == nullptr)
  {
    info.reset(new dp::BindingInfo(3));

    dp::BindingDecl & posDecl = info->GetBindingDecl(0);
    posDecl.m_attributeName = "a_position";
    posDecl.m_componentCount = 3;
    posDecl.m_componentType = gl_const::GLFloatType;
    posDecl.m_offset = 0;
    posDecl.m_stride = sizeof(StaticVertex);

    dp::BindingDecl & colorDecl = info->GetBindingDecl(1);
    colorDecl.m_attributeName = "a_colorTexCoord";
    colorDecl.m_componentCount = 2;
    colorDecl.m_componentType = gl_const::GLFloatType;
    colorDecl.m_offset = sizeof(glsl::vec3);
    colorDecl.m_stride = posDecl.m_stride;

    dp::BindingDecl & outlineDecl = info->GetBindingDecl(2);
    outlineDecl.m_attributeName = "a_outlineColorTexCoord";
    outlineDecl.m_componentCount = 2;
    outlineDecl.m_componentType = gl_const::GLFloatType;
    outlineDecl.m_offset = colorDecl.m_offset + sizeof(glsl::vec2);
    outlineDecl.m_stride = posDecl.m_stride;
  }

  return *info.get();
}

dp::BindingInfo const & GuiText::DynamicVertex::GetBindingInfo()
{
  static unique_ptr<dp::BindingInfo> info;

  if (info == nullptr)
  {
    info.reset(new dp::BindingInfo(2, 1));

    dp::BindingDecl & normalDecl = info->GetBindingDecl(0);
    normalDecl.m_attributeName = "a_normal";
    normalDecl.m_componentCount = 2;
    normalDecl.m_componentType = gl_const::GLFloatType;
    normalDecl.m_offset = 0;
    normalDecl.m_stride = sizeof(DynamicVertex);

    dp::BindingDecl & maskDecl = info->GetBindingDecl(1);
    maskDecl.m_attributeName = "a_maskTexCoord";
    maskDecl.m_componentCount = 2;
    maskDecl.m_componentType = gl_const::GLFloatType;
    maskDecl.m_offset = sizeof(glsl::vec2);
    maskDecl.m_stride = normalDecl.m_stride;
  }

  return *info.get();
}

GuiText::GuiText(dp::Anchor anchor)
  : m_anchor(anchor)
{
}

void GuiText::SetMaxLength(uint16_t maxLength)
{
  m_maxLength = maxLength;
}

dp::RefPointer<dp::Texture> GuiText::SetAlphabet(string const & alphabet, dp::RefPointer<dp::TextureManager> mng)
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

dp::RefPointer<dp::Texture> GuiText::Precache(buffer_vector<StaticVertex, 32> & buffer, dp::FontDecl const & font,
                                              dp::RefPointer<dp::TextureManager> mng)
{
  float const baseHeight = 20.0f;
  m_textRatio = font.m_size * DrapeGui::Instance().GetScaleFactor() / baseHeight;

  dp::TextureManager::ColorRegion color;
  dp::TextureManager::ColorRegion outlineColor;

  mng->GetColorRegion(font.m_color, color);
  mng->GetColorRegion(font.m_outlineColor, outlineColor);

  glsl::vec2 colorTex = glsl::ToVec2(color.GetTexRect().Center());
  glsl::vec2 outlineTex = glsl::ToVec2(outlineColor.GetTexRect().Center());
  float depth = 0.0f;

  for (size_t i = 0; i < m_maxLength; ++i)
  {
    buffer.push_back(StaticVertex(glsl::vec3(0.0, 0.0, depth), colorTex, outlineTex));
    buffer.push_back(StaticVertex(glsl::vec3(0.0, 0.0, depth), colorTex, outlineTex));
    buffer.push_back(StaticVertex(glsl::vec3(0.0, 0.0, depth), colorTex, outlineTex));
    buffer.push_back(StaticVertex(glsl::vec3(0.0, 0.0, depth), colorTex, outlineTex));
    depth += 10.0f;
  }

  return color.GetTexture();
}

void GuiText::SetText(buffer_vector<DynamicVertex, 32> & buffer, string text) const
{
  if (text.size() > m_maxLength)
    text = text.erase(m_maxLength - 3) + "...";

  strings::UniString uniText = fribidi::log2vis(strings::MakeUniString(text));

  float maxHeight = 0.0f;
  float length = 0.0f;
  glsl::vec2 offset = glsl::vec2(0.0, 0.0);

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
      dp::TextureManager::GlyphRegion const & glyph = it->second;
      m2::PointF pixelSize = m2::PointF(glyph.GetPixelSize()) * m_textRatio;
      m2::RectF const & r = glyph.GetTexRect();

      float xOffset = glyph.GetOffsetX() * m_textRatio;
      float yOffset = glyph.GetOffsetY() * m_textRatio;

      float const upVector = -static_cast<int32_t>(pixelSize.y) - yOffset;
      float const bottomVector = -yOffset;

      buffer.push_back(DynamicVertex(offset + glsl::vec2(xOffset, bottomVector), glsl::ToVec2(r.LeftTop())));
      buffer.push_back(DynamicVertex(offset + glsl::vec2(xOffset, upVector), glsl::ToVec2(r.LeftBottom())));
      buffer.push_back(DynamicVertex(offset + glsl::vec2(pixelSize.x + xOffset, bottomVector), glsl::ToVec2(r.RightTop())));
      buffer.push_back(DynamicVertex(offset + glsl::vec2(pixelSize.x + xOffset, upVector), glsl::ToVec2(r.RightBottom())));

      float const advance = glyph.GetAdvanceX() * m_textRatio;
      length += advance + xOffset;
      offset += glsl::vec2(advance, glyph.GetAdvanceY() * m_textRatio);
      maxHeight = max(maxHeight, (glyph.GetPixelHeight() + glyph.GetOffsetY()) * m_textRatio);
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
