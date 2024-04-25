#include "drape/glyph_manager.hpp"

#include "drape/font_constants.hpp"
#include "drape/glyph.hpp"

#include "platform/platform.hpp"

#include "coding/reader.hpp"

#include "base/string_utils.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/math.hpp"

#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <ft2build.h>
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
static int constexpr kInvalidFont = -1;

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
    FREETYPE_CHECK(FT_Render_Glyph(m_fontFace->glyph, FT_RENDER_MODE_SDF));

    FT_Glyph glyph;
    FREETYPE_CHECK(FT_Get_Glyph(m_fontFace->glyph, &glyph));

    FT_BBox bbox;
    FT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_PIXELS, &bbox);

    FT_Bitmap const bitmap = m_fontFace->glyph->bitmap;

    SharedBufferManager::shared_buffer_ptr_t data;
    if (bitmap.buffer != nullptr)
    {
      data = SharedBufferManager::instance().reserveSharedBuffer(bitmap.rows * bitmap.pitch);
      std::memcpy(data->data(), bitmap.buffer, data->size());
    }

    Glyph result;
    result.m_image = {bitmap.width, bitmap.rows, bitmap.rows, bitmap.pitch, data};
    // Glyph image has SDF borders that should be taken into an account.
    result.m_metrics = {float(glyph->advance.x >> 16), float(glyph->advance.y >> 16),
                        float(bbox.xMin + kSdfBorder), float(bbox.yMin + kSdfBorder), true};
    result.m_code = unicodePoint;
    FT_Done_Glyph(glyph);

    return result;
  }

  void GetCharcodes(std::vector<FT_ULong> & charcodes)
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

private:
  ReaderPtr<Reader> m_fontReader;
  FT_StreamRec_ m_stream;
  FT_Face m_fontFace;

  std::set<strings::UniChar> m_readyGlyphs;
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

  uint32_t m_baseGlyphHeight;
};

// Destructor is defined where pimpl's destructor is already known.
GlyphManager::~GlyphManager() = default;

GlyphManager::GlyphManager(Params const & params)
  : m_impl(std::make_unique<Impl>())
{
  m_impl->m_baseGlyphHeight = params.m_baseGlyphHeight;

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

uint32_t GlyphManager::GetBaseGlyphHeight() const
{
  return m_impl->m_baseGlyphHeight;
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
  Glyph glyph = f->GetGlyph(unicodePoint, m_impl->m_baseGlyphHeight);
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
    s_glyph = m_impl->m_fonts[kFontId]->GetGlyph(kInvalidGlyphCode, m_impl->m_baseGlyphHeight);
    s_glyph.m_metrics.m_isValid = false;
    s_glyph.m_fontIndex = kFontId;
    s_glyph.m_code = kInvalidGlyphCode;
    s_inited = true;
  }

  return s_glyph;
}
}  // namespace dp
