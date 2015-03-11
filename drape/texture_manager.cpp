#include "drape/texture_manager.hpp"
#include "drape/symbols_texture.hpp"
#include "drape/font_texture.hpp"
#include "drape/stipple_pen_resource.hpp"
#include "drape/texture_of_colors.hpp"

#include "drape/glfunctions.hpp"

#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"

#include "base/stl_add.hpp"

#include "std/vector.hpp"
#include "std/bind.hpp"

namespace dp
{

uint32_t const STIPPLE_TEXTURE_SIZE = 1024;
uint32_t const COLOR_TEXTURE_SIZE = 1024;
size_t const INVALID_GLYPH_GROUP = static_cast<size_t>(-1);

bool TextureManager::BaseRegion::IsValid() const
{
  return !m_info.IsNull() && !m_texture.IsNull();
}

void TextureManager::BaseRegion::SetResourceInfo(RefPointer<Texture::ResourceInfo> info)
{
  m_info = info;
}

void TextureManager::BaseRegion::SetTexture(RefPointer<Texture> texture)
{
  m_texture = texture;
}

m2::PointU TextureManager::BaseRegion::GetPixelSize() const
{
  ASSERT(IsValid(), ());
  m2::RectF const & texRect = m_info->GetTexRect();
  return  m2::PointU(ceil(texRect.SizeX() * m_texture->GetWidth()),
                     ceil(texRect.SizeY() * m_texture->GetHeight()));
}

uint32_t TextureManager::BaseRegion::GetPixelHeight() const
{
  return ceil(m_info->GetTexRect().SizeY() * m_texture->GetHeight());
}

m2::RectF const & TextureManager::BaseRegion::GetTexRect() const
{
  return m_info->GetTexRect();
}

TextureManager::GlyphRegion::GlyphRegion()
  : BaseRegion()
{
}

float TextureManager::GlyphRegion::GetOffsetX() const
{
  ASSERT(m_info->GetType() == Texture::Glyph, ());
  GlyphInfo const * info = static_cast<GlyphInfo const *>(m_info.GetRaw());
  return info->GetMetrics().m_xOffset;
}

float TextureManager::GlyphRegion::GetOffsetY() const
{
  ASSERT(m_info->GetType() == Texture::Glyph, ());
  GlyphInfo const * info = static_cast<GlyphInfo const *>(m_info.GetRaw());
  return info->GetMetrics().m_yOffset;
}

float TextureManager::GlyphRegion::GetAdvanceX() const
{
  ASSERT(m_info->GetType() == Texture::Glyph, ());
  GlyphInfo const * info = static_cast<GlyphInfo const *>(m_info.GetRaw());
  return info->GetMetrics().m_xAdvance;
}

float TextureManager::GlyphRegion::GetAdvanceY() const
{
  ASSERT(m_info->GetType() == Texture::Glyph, ());
  GlyphInfo const * info = static_cast<GlyphInfo const *>(m_info.GetRaw());
  return info->GetMetrics().m_yAdvance;
}

uint32_t TextureManager::StippleRegion::GetMaskPixelLength() const
{
  ASSERT(m_info->GetType() == Texture::StipplePen, ());
  return static_cast<StipplePenResourceInfo const *>(m_info.GetRaw())->GetMaskPixelLength();
}

uint32_t TextureManager::StippleRegion::GetPatternPixelLength() const
{
  ASSERT(m_info->GetType() == Texture::StipplePen, ());
  return static_cast<StipplePenResourceInfo const *>(m_info.GetRaw())->GetPatternPixelLength();
}

void TextureManager::UpdateDynamicTextures()
{
  if (m_nothingToUpload.test_and_set())
    return;

  m_colorTexture->UpdateState();
  m_stipplePenTexture->UpdateState();
  for_each(m_glyphGroups.begin(), m_glyphGroups.end(), [](GlyphGroup & g)
  {
    if (!g.m_texture.IsNull())
      g.m_texture->UpdateState();
  });

  for_each(m_hybridGlyphGroups.begin(), m_hybridGlyphGroups.end(), [](MasterPointer<Texture> texture)
  {
    texture->UpdateState();
  });
}

void TextureManager::AllocateGlyphTexture(TextureManager::GlyphGroup & group) const
{
  group.m_texture.Reset(new FontTexture(m2::PointU(m_maxTextureSize, m_maxTextureSize), m_glyphManager.GetRefPointer()));
}

void TextureManager::GetRegionBase(RefPointer<Texture> tex, TextureManager::BaseRegion & region, Texture::Key const & key)
{
  bool isNew = false;
  region.SetResourceInfo(tex->FindResource(key, isNew));
  region.SetTexture(tex);
  ASSERT(region.IsValid(), ());
  if (isNew)
    m_nothingToUpload.clear();
}

size_t TextureManager::FindCharGroup(strings::UniChar const & c)
{
  auto const iter = lower_bound(m_glyphGroups.begin(), m_glyphGroups.end(), c, [](GlyphGroup const & g, strings::UniChar const & c)
  {
    return g.m_endChar < c;
  });
  ASSERT(iter != m_glyphGroups.end(), ());
  return distance(m_glyphGroups.begin(), iter);
}

void TextureManager::FillResultBuffer(strings::UniString const & text, GlyphGroup & group, TGlyphsBuffer & regions)
{
  if (group.m_texture.IsNull())
    AllocateGlyphTexture(group);

  dp::RefPointer<dp::Texture> texture = group.m_texture.GetRefPointer();
  regions.reserve(text.size());
  for (strings::UniChar const & c : text)
  {
    GlyphRegion reg;
    GetRegionBase(texture, reg, GlyphKey(c));
    regions.push_back(reg);
  }
}

bool TextureManager::CheckCharGroup(strings::UniChar const & c, size_t & groupIndex)
{
  size_t currentIndex = FindCharGroup(c);
  if (groupIndex == INVALID_GLYPH_GROUP)
    groupIndex = currentIndex;
  else if (groupIndex != currentIndex)
  {
    groupIndex = INVALID_GLYPH_GROUP;
    return false;
  }

  return true;
}

TextureManager::TextureManager()
{
  m_nothingToUpload.test_and_set();
}

void TextureManager::Init(Params const & params)
{
  GLFunctions::glPixelStore(gl_const::GLUnpackAlignment, 1);
  SymbolsTexture * symbols = new SymbolsTexture();
  symbols->Load(my::JoinFoldersToPath(string("resources-") + params.m_resPrefix, "symbols"));
  m_symbolTexture.Reset(symbols);

  m_stipplePenTexture.Reset(new StipplePenTexture(m2::PointU(STIPPLE_TEXTURE_SIZE, STIPPLE_TEXTURE_SIZE)));
  m_colorTexture.Reset(new ColorTexture(m2::PointU(COLOR_TEXTURE_SIZE, COLOR_TEXTURE_SIZE)));

  m_glyphManager.Reset(new GlyphManager(params.m_glyphMngParams));
  m_maxTextureSize = min(2048, GLFunctions::glGetInteger(gl_const::GLMaxTextureSize));

  uint32_t const textureSquare = m_maxTextureSize * m_maxTextureSize;
  uint32_t const baseGlyphHeight = params.m_glyphMngParams.m_baseGlyphHeight;
  uint32_t const avarageGlyphSquare = baseGlyphHeight * baseGlyphHeight;

  m_glyphGroups.push_back(GlyphGroup());
  uint32_t glyphCount = ceil(0.9 * textureSquare / avarageGlyphSquare);
  m_glyphManager->ForEachUnicodeBlock([this, glyphCount](strings::UniChar const & start, strings::UniChar const & end)
  {
    if (m_glyphGroups.empty())
    {
      m_glyphGroups.push_back(GlyphGroup(start, end));
      return;
    }

    GlyphGroup & group = m_glyphGroups.back();
    ASSERT_LESS_OR_EQUAL(group.m_endChar, start, ());

    if (end - group.m_startChar < glyphCount)
      group.m_endChar = end;
    else
      m_glyphGroups.push_back(GlyphGroup(start, end));
  });
}

void TextureManager::Release()
{
  m_symbolTexture.Destroy();
  m_stipplePenTexture.Destroy();
  m_colorTexture.Destroy();

  DeleteRange(m_glyphGroups, [](GlyphGroup & g)
  {
    g.m_texture.Destroy();
  });

  DeleteRange(m_hybridGlyphGroups, MasterPointerDeleter());
}

void TextureManager::GetSymbolRegion(string const & symbolName, SymbolRegion & region)
{
  GetRegionBase(m_symbolTexture.GetRefPointer(), region, SymbolsTexture::SymbolKey(symbolName));
}

void TextureManager::GetStippleRegion(TStipplePattern const & pen, StippleRegion & region)
{
  GetRegionBase(m_stipplePenTexture.GetRefPointer(), region, StipplePenKey(pen));
}

void TextureManager::GetColorRegion(Color const & color, ColorRegion & region)
{
  GetRegionBase(m_colorTexture.GetRefPointer(), region, ColorKey(color));
}

void TextureManager::GetGlyphRegions(TMultilineText const & text,
                                     TMultilineGlyphsBuffer & buffers)
{
  size_t groupIndex = INVALID_GLYPH_GROUP;
  bool continueGroupFind = true;
  for (strings::UniString const & str : text)
  {
    for (strings::UniChar const & c : str)
    {
      continueGroupFind = CheckCharGroup(c, groupIndex);
      if (!continueGroupFind)
        break;
    }

    if (!continueGroupFind)
      break;
  }

  buffers.resize(text.size());
  if (groupIndex != INVALID_GLYPH_GROUP)
  {
    ASSERT_EQUAL(buffers.size(), text.size(), ());
    for (size_t i = 0; i < text.size(); ++i)
    {
      strings::UniString const & str = text[i];
      TGlyphsBuffer & buffer = buffers[i];
      FillResultBuffer(str, m_glyphGroups[groupIndex], buffer);
    }
  }
  else
  {
    /// TODO some magic with hybrid textures
  }
}

void TextureManager::GetGlyphRegions(strings::UniString const & text, TGlyphsBuffer & regions)
{
  size_t groupIndex = INVALID_GLYPH_GROUP;
  for (strings::UniChar const & c : text)
  {
    if (!CheckCharGroup(c, groupIndex))
      break;
  }

  regions.reserve(text.size());
  if (groupIndex != INVALID_GLYPH_GROUP)
    FillResultBuffer(text, m_glyphGroups[groupIndex], regions);
  else
  {
    /// TODO some magic with hybrid textures
  }
}

} // namespace dp
