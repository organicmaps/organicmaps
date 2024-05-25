#include "drape/glyph_manager.hpp"

#include "drape/font_constants.hpp"
#include "drape/glyph.hpp"
#include "drape/harfbuzz_shaping.hpp"

#include "platform/platform.hpp"

#include "coding/hex.hpp"
#include "coding/reader.hpp"
#include "coding/string_utf8_multilang.hpp"

#include "base/internal/message.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <ft2build.h>
#include <hb-ft.h>

#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include FT_FREETYPE_H
#include FT_MODULE_H
#include FT_SYSTEM_H
#include FT_STROKER_H
#include FT_TYPES_H

#undef __FTERRORS_H__
#define FT_ERRORDEF(e, v, s) {e, s},
#define FT_ERROR_START_LIST  {
#define FT_ERROR_END_LIST    {0, 0}};
struct FreetypeError
{
  int m_code;
  char const * const m_message;
};

FreetypeError constexpr g_FT_Errors[] =
#include FT_ERRORS_H

#ifdef DEBUG
#define FREETYPE_CHECK(x) \
    do \
    { \
      FT_Error const err = (x); \
      if (err) \
        LOG(LERROR, ("Freetype:", g_FT_Errors[err].m_code, g_FT_Errors[err].m_message)); \
    } while (false)
#else
#define FREETYPE_CHECK(x) x
#endif

namespace dp
{
int constexpr kInvalidFont = -1;

template <typename ToDo>
void ParseUniBlocks(std::string const & uniBlocksFile, ToDo toDo)
{
  std::string uniBlocks;
  try
  {
    ReaderPtr<Reader>(GetPlatform().GetReader(uniBlocksFile)).ReadAsString(uniBlocks);
  }
  catch (RootException const & e)
  {
    LOG(LCRITICAL, ("Error reading uniblock description: ", e.what()));
    return;
  }

  std::istringstream fin(uniBlocks);
  while (true)
  {
    std::string name;
    uint32_t start, end;
    fin >> name >> std::hex >> start >> std::hex >> end;
    if (!fin)
      break;

    toDo(name, start, end);
  }
}

template <typename ToDo>
void ParseFontList(std::string const & fontListFile, ToDo toDo)
{
  std::string fontList;
  try
  {
    ReaderPtr<Reader>(GetPlatform().GetReader(fontListFile)).ReadAsString(fontList);
  }
  catch(RootException const & e)
  {
    LOG(LWARNING, ("Error reading font list ", fontListFile, " : ", e.what()));
    return;
  }

  std::istringstream fin(fontList);
  while (true)
  {
    std::string ubName;
    std::string fontName;
    fin >> ubName >> fontName;
    if (!fin)
      break;

    toDo(ubName, fontName);
  }
}

class Font
{
public:
  DECLARE_EXCEPTION(InvalidFontException, RootException);
  DISALLOW_COPY_AND_MOVE(Font);

  Font(ReaderPtr<Reader> && fontReader, FT_Library lib)
    : m_fontReader(std::move(fontReader))
    , m_fontFace(nullptr)
  {
    std::memset(&m_stream, 0, sizeof(m_stream));
    m_stream.size = static_cast<unsigned long>(m_fontReader.Size());
    m_stream.descriptor.pointer = &m_fontReader;
    m_stream.read = &Font::Read;
    m_stream.close = &Font::Close;

    FT_Open_Args args = {};
    args.flags = FT_OPEN_STREAM;
    args.stream = &m_stream;

    FT_Error const err = FT_Open_Face(lib, &args, 0, &m_fontFace);
    if (err || !IsValid())
      MYTHROW(InvalidFontException, (g_FT_Errors[err].m_code, g_FT_Errors[err].m_message));
  }

  ~Font()
  {
    ASSERT(m_fontFace, ());
    if (m_harfbuzzFont)
      hb_font_destroy(m_harfbuzzFont);

    FREETYPE_CHECK(FT_Done_Face(m_fontFace));
  }

  bool IsValid() const { return m_fontFace && m_fontFace->num_glyphs > 0; }

  bool HasGlyph(strings::UniChar unicodePoint) const { return FT_Get_Char_Index(m_fontFace, unicodePoint) != 0; }

  Glyph GetGlyph(strings::UniChar unicodePoint, uint32_t glyphHeight) const
  {
    FREETYPE_CHECK(FT_Set_Pixel_Sizes(m_fontFace, 0, glyphHeight));

    // FT_LOAD_RENDER with FT_RENDER_MODE_SDF uses bsdf driver that is around 3x times faster
    // than sdf driver, activated by FT_LOAD_DEFAULT + FT_RENDER_MODE_SDF
    FREETYPE_CHECK(FT_Load_Glyph(m_fontFace, FT_Get_Char_Index(m_fontFace, unicodePoint), FT_LOAD_RENDER));
    FT_GlyphSlot const & glyph = m_fontFace->glyph;
    FREETYPE_CHECK(FT_Render_Glyph(glyph, FT_RENDER_MODE_SDF));

    FT_Bitmap const bitmap = glyph->bitmap;

    SharedBufferManager::shared_buffer_ptr_t data;
    if (bitmap.buffer != nullptr)
    {
      data = SharedBufferManager::instance().reserveSharedBuffer(bitmap.rows * bitmap.pitch);
      std::memcpy(data->data(), bitmap.buffer, data->size());
    }

    // Glyph image has SDF borders that should be taken into an account.
    float const xAdvance = glyph->advance.x >> 6;
    float const yAdvance = glyph->advance.y >> 6;
    float const xOffset = glyph->metrics.horiBearingX >> 6;
    // yOffset uses bottom left coordinate in calculations.
    float const yOffset = (glyph->metrics.horiBearingY - glyph->metrics.height) >> 6;

    Glyph result {
      .m_metrics = {xAdvance, yAdvance, xOffset, yOffset, true},
      .m_image = {bitmap.width, bitmap.rows, data},
      .m_code = unicodePoint,
    };
    return result;
  }

  GlyphImage GetGlyphImage(uint16_t glyphId, int pixelHeight, bool sdf) const
  {
    FREETYPE_CHECK(FT_Set_Pixel_Sizes(m_fontFace, 0, pixelHeight));
    // FT_LOAD_RENDER with FT_RENDER_MODE_SDF uses bsdf driver that is around 3x times faster
    // than sdf driver, activated by FT_LOAD_DEFAULT + FT_RENDER_MODE_SDF
    FREETYPE_CHECK(FT_Load_Glyph(m_fontFace, glyphId, FT_LOAD_RENDER));
    if (sdf)
      FREETYPE_CHECK(FT_Render_Glyph(m_fontFace->glyph, FT_RENDER_MODE_SDF));

    FT_Bitmap const & bitmap = m_fontFace->glyph->bitmap;

    SharedBufferManager::shared_buffer_ptr_t data;
    if (bitmap.buffer != nullptr)
    {
      // Bitmap is stored without a padding.
      data = SharedBufferManager::instance().reserveSharedBuffer(bitmap.width * bitmap.rows);
      auto ptr = data->data();

      for (unsigned int row = 0; row < bitmap.rows; ++row)
      {
        unsigned int const dstBaseIndex = row * bitmap.width;
        int const srcBaseIndex = static_cast<int>(row) * bitmap.pitch;
        for (unsigned int column = 0; column < bitmap.width; ++column)
          ptr[dstBaseIndex + column] = bitmap.buffer[srcBaseIndex + column];
      }
    }
    return {bitmap.width, bitmap.rows, data};
  }

  void GetCharcodes(std::vector<FT_ULong> & charcodes) const
  {
    FT_UInt gindex;
    charcodes.push_back(FT_Get_First_Char(m_fontFace, &gindex));
    while (gindex)
      charcodes.push_back(FT_Get_Next_Char(m_fontFace, charcodes.back(), &gindex));

    base::SortUnique(charcodes);
  }

  static unsigned long Read(FT_Stream stream, unsigned long offset, unsigned char * buffer, unsigned long count)
  {
    if (count != 0)
    {
      auto * reader = reinterpret_cast<ReaderPtr<Reader> *>(stream->descriptor.pointer);
      reader->Read(offset, buffer, count);
    }

    return count;
  }

  static void Close(FT_Stream) {}

  void MarkGlyphReady(strings::UniChar code)
  {
    m_readyGlyphs.emplace(code);
  }

  bool IsGlyphReady(strings::UniChar code) const
  {
    return m_readyGlyphs.find(code) != m_readyGlyphs.end();
  }

  std::string GetName() const { return std::string(m_fontFace->family_name) + ':' + m_fontFace->style_name; }

  void Shape(hb_buffer_t * hbBuffer, int fontPixelSize, int fontIndex, text::TextMetrics & outMetrics)
  {
    // TODO(AB): This code is not thread safe.

    // TODO(AB): Do not set the same font size every time.
    FREETYPE_CHECK(FT_Set_Pixel_Sizes(m_fontFace, 0 /* pixel_width */, fontPixelSize /* pixel_height */));
    if (!m_harfbuzzFont)
      m_harfbuzzFont = hb_ft_font_create(m_fontFace, nullptr);
    //else
    // Call on each font size change.
    //  hb_ft_font_changed(m_harfbuzzFont);

    // Shape!
    hb_shape(m_harfbuzzFont, hbBuffer, nullptr, 0);

    // Get the glyph and position information.
    unsigned int glyphCount;
    hb_glyph_info_t const * glyphInfo = hb_buffer_get_glyph_infos(hbBuffer, &glyphCount);
    hb_glyph_position_t const * glyphPos = hb_buffer_get_glyph_positions(hbBuffer, &glyphCount);

    for (unsigned int i = 0; i < glyphCount; ++i)
    {
      // TODO(AB): Check for missing glyph ID?
      auto const glyphId = static_cast<uint16_t>(glyphInfo[i].codepoint);

      // TODO(AB): Load each glyph only once for the given font size? Or is run cache more efficient?
      FT_Int32 constexpr flags = FT_LOAD_DEFAULT;
      FREETYPE_CHECK(FT_Load_Glyph(m_fontFace, glyphId, flags));

      auto const & currPos = glyphPos[i];
      // TODO(AB): Use floats for subpixel precision?
      hb_position_t const xOffset  = (currPos.x_offset + static_cast<int32_t>(m_fontFace->glyph->metrics.horiBearingX)) >> 6;
      hb_position_t const yOffset  = (currPos.y_offset + static_cast<int32_t>(m_fontFace->glyph->metrics.horiBearingY)) >> 6;
      hb_position_t const xAdvance = currPos.x_advance >> 6;
      auto const height = static_cast<int32_t>(m_fontFace->glyph->metrics.height >> 6);

      outMetrics.AddGlyphMetrics(static_cast<int16_t>(fontIndex), glyphId, xOffset, yOffset, xAdvance, height);
    }
  }

private:
  ReaderPtr<Reader> m_fontReader;
  FT_StreamRec_ m_stream;
  FT_Face m_fontFace;

  std::set<strings::UniChar> m_readyGlyphs;

  hb_font_t* m_harfbuzzFont {nullptr};
};

// Information about single unicode block.
struct UnicodeBlock
{
  std::string m_name;

  strings::UniChar m_start;
  strings::UniChar m_end;
  std::vector<int> m_fontsWeight;

  UnicodeBlock(std::string const & name, strings::UniChar start, strings::UniChar end)
    : m_name(name)
    , m_start(start)
    , m_end(end)
  {}

  int GetFontOffset(int idx) const
  {
    if (m_fontsWeight.empty())
      return kInvalidFont;

    int maxWeight = 0;
    int upperBoundWeight = std::numeric_limits<int>::max();
    if (idx != kInvalidFont)
      upperBoundWeight = m_fontsWeight[idx];

    int index = kInvalidFont;
    ASSERT_LESS(m_fontsWeight.size(), static_cast<size_t>(std::numeric_limits<int>::max()), ());
    for (size_t i = 0; i < m_fontsWeight.size(); ++i)
    {
      int const w = m_fontsWeight[i];
      if (w < upperBoundWeight && w > maxWeight)
      {
        maxWeight = w;
        index = static_cast<int>(i);
      }
    }

    return index;
  }

  bool HasSymbol(strings::UniChar sym) const
  {
    return (m_start <= sym) && (m_end >= sym);
  }
};

using TUniBlocks = std::vector<UnicodeBlock>;
using TUniBlockIter = TUniBlocks::const_iterator;

struct GlyphManager::Impl
{
  DISALLOW_COPY_AND_MOVE(Impl);

  Impl() = default;

  ~Impl()
  {
    m_fonts.clear();
    if (m_library)
      FREETYPE_CHECK(FT_Done_FreeType(m_library));
  }

  FT_Library m_library;
  TUniBlocks m_blocks;
  TUniBlockIter m_lastUsedBlock;
  std::vector<std::unique_ptr<Font>> m_fonts;
};

// Destructor is defined where pimpl's destructor is already known.
GlyphManager::~GlyphManager() = default;

GlyphManager::GlyphManager(Params const & params)
  : m_impl(std::make_unique<Impl>())
{
  using TFontAndBlockName = std::pair<std::string, std::string>;
  using TFontLst = buffer_vector<TFontAndBlockName, 64>;

  TFontLst whitelst;
  TFontLst blacklst;

  m_impl->m_blocks.reserve(160);
  ParseUniBlocks(params.m_uniBlocks, [this](std::string const & name,
                                            strings::UniChar start, strings::UniChar end)
  {
    m_impl->m_blocks.emplace_back(name, start, end);
  });

  ParseFontList(params.m_whitelist, [&whitelst](std::string const & ubName, std::string const & fontName)
  {
    whitelst.emplace_back(fontName, ubName);
  });

  ParseFontList(params.m_blacklist, [&blacklst](std::string const & ubName, std::string const & fontName)
  {
    blacklst.emplace_back(fontName, ubName);
  });

  m_impl->m_fonts.reserve(params.m_fonts.size());

  FREETYPE_CHECK(FT_Init_FreeType(&m_impl->m_library));

  // Default Freetype spread/sdf border is 8.
  static constexpr FT_Int kSdfBorder = dp::kSdfBorder;
  for (auto const module : {"sdf", "bsdf"})
    FREETYPE_CHECK(FT_Property_Set(m_impl->m_library, module, "spread", &kSdfBorder));

  for (auto const & fontName : params.m_fonts)
  {
    bool ignoreFont = false;
    std::for_each(blacklst.begin(), blacklst.end(), [&ignoreFont, &fontName](TFontAndBlockName const & p)
    {
      if (p.first == fontName && p.second == "*")
        ignoreFont = true;
    });

    if (ignoreFont)
      continue;

    std::vector<FT_ULong> charCodes;
    try
    {
      m_impl->m_fonts.emplace_back(std::make_unique<Font>(GetPlatform().GetReader(fontName), m_impl->m_library));
      m_impl->m_fonts.back()->GetCharcodes(charCodes);
    }
    catch(RootException const & e)
    {
      LOG(LWARNING, ("Error reading font file =", fontName, "; Reason =", e.what()));
      continue;
    }

    using BlockIndex = size_t;
    using CharCounter = int;
    using CoverNode = std::pair<BlockIndex, CharCounter>;
    using CoverInfo = std::vector<CoverNode>;

    size_t currentUniBlock = 0;
    CoverInfo coverInfo;
    for (auto const charCode : charCodes)
    {
      size_t block = currentUniBlock;
      while (block < m_impl->m_blocks.size())
      {
        if (m_impl->m_blocks[block].HasSymbol(static_cast<strings::UniChar>(charCode)))
          break;
        ++block;
      }

      if (block < m_impl->m_blocks.size())
      {
        if (coverInfo.empty() || coverInfo.back().first != block)
          coverInfo.emplace_back(block, 1);
        else
          ++coverInfo.back().second;

        currentUniBlock = block;
      }
    }

    using TUpdateCoverInfoFn = std::function<void(UnicodeBlock const & uniBlock, CoverNode & node)>;
    auto const enumerateFn = [this, &coverInfo, &fontName] (TFontLst const & lst, TUpdateCoverInfoFn const & fn)
    {
      for (auto const & b : lst)
      {
        if (b.first != fontName)
          continue;

        for (CoverNode & node : coverInfo)
        {
          auto const & uniBlock = m_impl->m_blocks[node.first];
          if (uniBlock.m_name == b.second)
          {
            fn(uniBlock, node);
            break;
          }
          else if (b.second == "*")
          {
            fn(uniBlock, node);
          }
        }
      }
    };

    enumerateFn(blacklst, [](UnicodeBlock const &, CoverNode & node)
    {
      node.second = 0;
    });

    enumerateFn(whitelst, [this](UnicodeBlock const & uniBlock, CoverNode & node)
    {
      node.second = static_cast<int>(uniBlock.m_end + 1 - uniBlock.m_start + m_impl->m_fonts.size());
    });

    for (CoverNode const & node : coverInfo)
    {
      UnicodeBlock & uniBlock = m_impl->m_blocks[node.first];
      uniBlock.m_fontsWeight.resize(m_impl->m_fonts.size(), 0);
      uniBlock.m_fontsWeight.back() = node.second;
    }
  }

  m_impl->m_lastUsedBlock = m_impl->m_blocks.end();

  LOG(LDEBUG, ("How unicode blocks are mapped on font files:"));

  // We don't have black list for now.
  ASSERT_EQUAL(m_impl->m_fonts.size(), params.m_fonts.size(), ());

  for (auto const & b : m_impl->m_blocks)
  {
    auto const & weights = b.m_fontsWeight;
    ASSERT_LESS_OR_EQUAL(weights.size(), m_impl->m_fonts.size(), ());
    if (weights.empty())
    {
      LOG_SHORT(LDEBUG, (b.m_name, "is unsupported"));
    }
    else
    {
      size_t const ind = std::distance(weights.begin(), std::max_element(weights.begin(), weights.end()));
      LOG_SHORT(LDEBUG, (b.m_name, "is in", params.m_fonts[ind]));
    }
  }
}

int GlyphManager::GetFontIndex(strings::UniChar unicodePoint)
{
  auto iter = m_impl->m_blocks.cend();
  if (m_impl->m_lastUsedBlock != m_impl->m_blocks.end() &&
      m_impl->m_lastUsedBlock->HasSymbol(unicodePoint))
  {
    iter = m_impl->m_lastUsedBlock;
  }
  else
  {
    if (iter == m_impl->m_blocks.end() || !iter->HasSymbol(unicodePoint))
    {
      iter = std::lower_bound(m_impl->m_blocks.begin(), m_impl->m_blocks.end(), unicodePoint,
                              [](UnicodeBlock const & block, strings::UniChar const & v)
      {
        return block.m_end < v;
      });
    }
  }

  if (iter == m_impl->m_blocks.end() || !iter->HasSymbol(unicodePoint))
    return kInvalidFont;

  m_impl->m_lastUsedBlock = iter;

  return FindFontIndexInBlock(*m_impl->m_lastUsedBlock, unicodePoint);
}

int GlyphManager::GetFontIndex(std::u16string_view sv)
{
  // Only get font for the first character.
  // TODO(AB): Make sure that text runs are split by fonts.
  auto it = sv.begin();
  return GetFontIndex(utf8::unchecked::next16(it));
}

int GlyphManager::GetFontIndexImmutable(strings::UniChar unicodePoint) const
{
  TUniBlockIter iter = std::lower_bound(m_impl->m_blocks.begin(), m_impl->m_blocks.end(), unicodePoint,
                                        [](UnicodeBlock const & block, strings::UniChar const & v)
  {
    return block.m_end < v;
  });

  if (iter == m_impl->m_blocks.end() || !iter->HasSymbol(unicodePoint))
    return kInvalidFont;

  return FindFontIndexInBlock(*iter, unicodePoint);
}

int GlyphManager::FindFontIndexInBlock(UnicodeBlock const & block, strings::UniChar unicodePoint) const
{
  ASSERT(block.HasSymbol(unicodePoint), ());
  for (int fontIndex = block.GetFontOffset(kInvalidFont); fontIndex != kInvalidFont;
       fontIndex = block.GetFontOffset(fontIndex))
  {
    ASSERT_LESS(fontIndex, static_cast<int>(m_impl->m_fonts.size()), ());
    auto const & f = m_impl->m_fonts[fontIndex];
    if (f->HasGlyph(unicodePoint))
      return fontIndex;
  }

  return kInvalidFont;
}

Glyph GlyphManager::GetGlyph(strings::UniChar unicodePoint)
{
  int const fontIndex = GetFontIndex(unicodePoint);
  if (fontIndex == kInvalidFont)
    return GetInvalidGlyph();

  auto const & f = m_impl->m_fonts[fontIndex];
  Glyph glyph = f->GetGlyph(unicodePoint, kBaseFontSizePixels);
  glyph.m_fontIndex = fontIndex;
  return glyph;
}

void GlyphManager::MarkGlyphReady(Glyph const & glyph)
{
  ASSERT_GREATER_OR_EQUAL(glyph.m_fontIndex, 0, ());
  ASSERT_LESS(glyph.m_fontIndex, static_cast<int>(m_impl->m_fonts.size()), ());
  m_impl->m_fonts[glyph.m_fontIndex]->MarkGlyphReady(glyph.m_code);
}

bool GlyphManager::AreGlyphsReady(strings::UniString const & str) const
{
  for (auto const & code : str)
  {
    int const fontIndex = GetFontIndexImmutable(code);
    if (fontIndex == kInvalidFont)
      return false;

    if (!m_impl->m_fonts[fontIndex]->IsGlyphReady(code))
      return false;
  }

  return true;
}

Glyph const & GlyphManager::GetInvalidGlyph() const
{
  static bool s_inited = false;
  static Glyph s_glyph;

  if (!s_inited)
  {
    strings::UniChar constexpr kInvalidGlyphCode = 0x9;
    int constexpr kFontId = 0;
    ASSERT(!m_impl->m_fonts.empty(), ());
    s_glyph = m_impl->m_fonts[kFontId]->GetGlyph(kInvalidGlyphCode, kBaseFontSizePixels);
    s_glyph.m_metrics.m_isValid = false;
    s_glyph.m_fontIndex = kFontId;
    s_glyph.m_code = kInvalidGlyphCode;
    s_inited = true;
  }

  return s_glyph;
}

GlyphImage GlyphManager::GetGlyphImage(int fontIndex, uint16_t glyphId, int pixelHeight, bool sdf)
{
  return m_impl->m_fonts[fontIndex]->GetGlyphImage(glyphId, pixelHeight, sdf);
}

namespace
{
hb_language_t OrganicMapsLanguageToHarfbuzzLanguage(int8_t lang)
{
  // TODO(AB): can langs be converted faster?
  auto const svLang = StringUtf8Multilang::GetLangByCode(lang);
  auto const hbLanguage = hb_language_from_string(svLang.data(), static_cast<int>(svLang.size()));
  if (hbLanguage == HB_LANGUAGE_INVALID)
    return hb_language_get_default();
  return hbLanguage;
}
}  // namespace

text::TextMetrics GlyphManager::ShapeText(std::string_view utf8, int fontPixelHeight, int8_t lang)
{
  const auto [text, segments] = harfbuzz_shaping::GetTextSegments(utf8);

  // TODO(AB): Optimize language conversion.
  hb_language_t const hbLanguage = OrganicMapsLanguageToHarfbuzzLanguage(lang);

  text::TextMetrics allGlyphs;

  // TODO(AB): Cache runs.
  // TODO(AB): Cache buffer.
  hb_buffer_t * buf = hb_buffer_create();
  for (auto const & substring : segments)
  {
    hb_buffer_clear_contents(buf);

    // TODO(AB): Some substrings use different fonts.
    hb_buffer_add_utf16(buf, reinterpret_cast<const uint16_t *>(text.data()), static_cast<int>(text.size()),
                        substring.m_start, substring.m_length);
    hb_buffer_set_direction(buf, substring.m_direction);
    hb_buffer_set_script(buf, substring.m_script);
    hb_buffer_set_language(buf, hbLanguage);

    // TODO(AB): Check not only the first character to determine font for the run, but all chars.
    auto firstCharacterIter{text.begin() + substring.m_start};
    auto const firstCharacterUnicode = utf8::unchecked::next16(firstCharacterIter);

    int const fontIndex = GetFontIndex(firstCharacterUnicode);
    if (fontIndex < 0)
    {
      // TODO(AB): Add missing glyph character's metrics
      LOG(LWARNING, ("Skip run because no font was found for character", NumToHex(firstCharacterUnicode)));
      continue;
    }

    m_impl->m_fonts[fontIndex]->Shape(buf, fontPixelHeight, fontIndex, allGlyphs);
  }
  // Tidy up.
  // TODO(AB): avoid recreating buffer each time.
  hb_buffer_destroy(buf);

  return allGlyphs;
}

text::TextMetrics GlyphManager::ShapeText(std::string_view utf8, int fontPixelHeight, char const * lang)
{
  return ShapeText(utf8, fontPixelHeight, StringUtf8Multilang::GetLangIndex(lang));
}

}  // namespace dp
