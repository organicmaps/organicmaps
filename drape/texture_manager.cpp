#include "drape/texture_manager.hpp"
#include "drape/symbols_texture.hpp"
#include "drape/font_texture.hpp"
#include "drape/stipple_pen_resource.hpp"
#include "drape/texture_of_colors.hpp"
#include "drape/glfunctions.hpp"
#include "drape/utils/glyph_usage_tracker.hpp"

#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"

#include "base/stl_add.hpp"

#include "std/vector.hpp"
#include "std/bind.hpp"

namespace dp
{

uint32_t const STIPPLE_TEXTURE_SIZE = 1024;
uint32_t const COLOR_TEXTURE_SIZE = 1024;
size_t const INVALID_GLYPH_GROUP = numeric_limits<size_t>::max();

// number of glyphs (since 0) which will be in each texture
size_t const DUPLICATED_GLYPHS_COUNT = 128;

TextureManager::TextureManager()
  : m_maxTextureSize(0)
  , m_maxGlypsCount(0)
{
  m_nothingToUpload.test_and_set();
}

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

  for_each(m_hybridGlyphGroups.begin(), m_hybridGlyphGroups.end(), [](HybridGlyphGroup & g)
  {
    if (!g.m_texture.IsNull())
      g.m_texture->UpdateState();
  });
}

MasterPointer<Texture> TextureManager::AllocateGlyphTexture() const
{
  return MasterPointer<Texture>(new FontTexture(m2::PointU(m_maxTextureSize, m_maxTextureSize), m_glyphManager.GetRefPointer()));
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

size_t TextureManager::FindGlyphsGroup(strings::UniChar const & c) const
{
  auto const iter = lower_bound(m_glyphGroups.begin(), m_glyphGroups.end(), c, [](GlyphGroup const & g, strings::UniChar const & c)
  {
    return g.m_endChar < c;
  });

  if (iter == m_glyphGroups.end())
    return INVALID_GLYPH_GROUP;

  return distance(m_glyphGroups.begin(), iter);
}

size_t TextureManager::FindGlyphsGroup(strings::UniString const & text, size_t const defaultGroup) const
{
  size_t groupIndex = defaultGroup;
  for (strings::UniChar const & c : text)
  {
    // skip glyphs which can be duplicated
    if (c < DUPLICATED_GLYPHS_COUNT)
      continue;

    size_t currentIndex = FindGlyphsGroup(c);

    // an invalid glyph found
    if (currentIndex == INVALID_GLYPH_GROUP)
    {
#if defined(TRACK_GLYPH_USAGE)
      GlyphUsageTracker::Instance().AddInvalidGlyph(text, c);
#endif
      return INVALID_GLYPH_GROUP;
    }

    // check if each glyph in text id in one group
    if (groupIndex == INVALID_GLYPH_GROUP)
      groupIndex = currentIndex;
    else if (groupIndex != currentIndex)
    {
#if defined(TRACK_GLYPH_USAGE)
      GlyphUsageTracker::Instance().AddUnexpectedGlyph(text, c, currentIndex, groupIndex);
#endif
      return INVALID_GLYPH_GROUP;
    }
  }

  // all glyphs in duplicated range
  if (groupIndex == INVALID_GLYPH_GROUP)
    groupIndex = FindGlyphsGroup(text[0]);

  return groupIndex;
}

size_t TextureManager::FindGlyphsGroup(TMultilineText const & text) const
{
  size_t groupIndex = INVALID_GLYPH_GROUP;
  for (strings::UniString const & str : text)
  {
    size_t currentIndex = FindGlyphsGroup(str, groupIndex);
    if (currentIndex == INVALID_GLYPH_GROUP)
      return INVALID_GLYPH_GROUP;

    if (groupIndex == INVALID_GLYPH_GROUP)
      groupIndex = currentIndex;
    else if (groupIndex != currentIndex)
      return INVALID_GLYPH_GROUP;
  }

  return groupIndex;
}

bool TextureManager::CheckHybridGroup(strings::UniString const & text, HybridGlyphGroup const & group) const
{
  for (strings::UniChar const & c : text)
    if (group.m_glyphs.find(c) == group.m_glyphs.end())
      return false;

  return true;
}

size_t TextureManager::GetNumberOfUnfoundCharacters(strings::UniString const & text, HybridGlyphGroup const & group) const
{
  size_t cnt = 0;
  for (strings::UniChar const & c : text)
    if (group.m_glyphs.find(c) == group.m_glyphs.end())
      cnt++;

  return cnt;
}

void TextureManager::MarkCharactersUsage(strings::UniString const & text, HybridGlyphGroup & group)
{
  for (strings::UniChar const & c : text)
    group.m_glyphs.emplace(c);
}

size_t TextureManager::FindHybridGlyphsGroup(strings::UniString const & text)
{
  if (m_hybridGlyphGroups.empty())
  {
    m_hybridGlyphGroups.push_back(HybridGlyphGroup());
    return 0;
  }

  // if we have got the only hybrid texture (in most cases it is) we can omit checking of glyphs usage
  size_t const glyphsCount = m_hybridGlyphGroups.back().m_glyphs.size() + text.size();
  if (m_hybridGlyphGroups.size() == 1 && glyphsCount < m_maxGlypsCount)
    return 0;

  // looking for a hybrid texture which contains text entirely
  for (size_t i = 0; i < m_hybridGlyphGroups.size() - 1; i++)
    if (CheckHybridGroup(text, m_hybridGlyphGroups[i]))
      return i;

  // check if we can contain text in the last hybrid texture
  size_t const unfoundChars = GetNumberOfUnfoundCharacters(text, m_hybridGlyphGroups.back());
  size_t const newCharsCount = m_hybridGlyphGroups.back().m_glyphs.size() + unfoundChars;
  if (newCharsCount < m_maxGlypsCount)
    return m_hybridGlyphGroups.size() - 1;

  // nothing helps insert new hybrid group
  m_hybridGlyphGroups.push_back(HybridGlyphGroup());
  return m_hybridGlyphGroups.size() - 1;
}

size_t TextureManager::FindHybridGlyphsGroup(TMultilineText const & text)
{
  size_t cnt = 0;
  for (strings::UniString const & str : text)
    cnt += str.size();

  strings::UniString combinedString;
  combinedString.reserve(cnt);
  for (strings::UniString const & str : text)
    combinedString.append(str.begin(), str.end());

  return FindHybridGlyphsGroup(combinedString);
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
  m_maxGlypsCount = ceil(0.9 * textureSquare / avarageGlyphSquare);
  m_glyphManager->ForEachUnicodeBlock([this](strings::UniChar const & start, strings::UniChar const & end)
  {
    if (m_glyphGroups.empty())
    {
      m_glyphGroups.push_back(GlyphGroup(start, end));
      return;
    }

    GlyphGroup & group = m_glyphGroups.back();
    ASSERT_LESS_OR_EQUAL(group.m_endChar, start, ());

    if (end - group.m_startChar < m_maxGlypsCount)
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

  DeleteRange(m_hybridGlyphGroups, [](HybridGlyphGroup & g)
  {
    g.m_texture.Destroy();
  });
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
  size_t groupIndex = FindGlyphsGroup(text);

  buffers.resize(text.size());
  ASSERT_EQUAL(buffers.size(), text.size(), ());
  if (groupIndex != INVALID_GLYPH_GROUP)
  {
    for (size_t i = 0; i < text.size(); ++i)
    {
      strings::UniString const & str = text[i];
      TGlyphsBuffer & buffer = buffers[i];
      FillResultBuffer<GlyphGroup, GlyphKey>(str, m_glyphGroups[groupIndex], buffer);
    }
  }
  else
  {
    size_t hybridGroupIndex = FindHybridGlyphsGroup(text);
    ASSERT(hybridGroupIndex != INVALID_GLYPH_GROUP, (""));

    for (size_t i = 0; i < text.size(); ++i)
    {
      strings::UniString const & str = text[i];
      TGlyphsBuffer & buffer = buffers[i];

      MarkCharactersUsage(str, m_hybridGlyphGroups[hybridGroupIndex]);
      FillResultBuffer<HybridGlyphGroup, GlyphKey>(str, m_hybridGlyphGroups[hybridGroupIndex], buffer);
    }
  }
}

void TextureManager::GetGlyphRegions(strings::UniString const & text, TGlyphsBuffer & regions)
{
  size_t groupIndex = FindGlyphsGroup(text, INVALID_GLYPH_GROUP);

  regions.reserve(text.size());
  if (groupIndex != INVALID_GLYPH_GROUP)
    FillResultBuffer<GlyphGroup, GlyphKey>(text, m_glyphGroups[groupIndex], regions);
  else
  {
    size_t hybridGroupIndex = FindHybridGlyphsGroup(text);
    ASSERT(hybridGroupIndex != INVALID_GLYPH_GROUP, (""));

    MarkCharactersUsage(text, m_hybridGlyphGroups[hybridGroupIndex]);
    FillResultBuffer<HybridGlyphGroup, GlyphKey>(text, m_hybridGlyphGroups[hybridGroupIndex], regions);
  }
}

} // namespace dp
