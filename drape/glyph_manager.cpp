#include "drape/glyph_manager.hpp"
#include "3party/sdf_image/sdf_image.h"

#include "platform/platform.hpp"

#include "coding/reader.hpp"

#include "base/string_utils.hpp"
#include "base/logging.hpp"
#include "base/math.hpp"
#include "base/timer.hpp"

#include "std/unique_ptr.hpp"
#include "std/unordered_set.hpp"

#include <ft2build.h>
#include FT_TYPES_H
#include FT_SYSTEM_H
#include FT_FREETYPE_H
#include FT_STROKER_H
#include FT_CACHE_H

#ifdef DEBUG
  #undef __FTERRORS_H__
  #define FT_ERRORDEF(e, v, s) {e, s},
  #define FT_ERROR_START_LIST  {
  #define FT_ERROR_END_LIST    {0, 0}};
  struct FreetypeError
  {
    int m_code;
    char const * m_message;
  };

  FreetypeError g_FT_Errors[] =
  #include FT_ERRORS_H

  #define FREETYPE_CHECK(x) \
    do \
    { \
      FT_Error const err = (x); \
      if (err) \
        LOG(LWARNING, ("Freetype:", g_FT_Errors[err].m_code, g_FT_Errors[err].m_message)); \
    } while (false)

  #define FREETYPE_CHECK_RETURN(x, msg) \
    do \
    { \
      FT_Error const err = (x); \
      if (err) \
      { \
        LOG(LWARNING, ("Freetype", g_FT_Errors[err].m_code, g_FT_Errors[err].m_message, msg)); \
        return; \
      } \
    } while (false)
#else
  #define FREETYPE_CHECK(x) x
  #define FREETYPE_CHECK_RETURN(x, msg) FREETYPE_CHECK(x)
#endif

namespace dp
{

namespace
{

uint32_t const kSdfBorder = 4;
int const kInvalidFont = -1;

template <typename ToDo>
void ParseUniBlocks(string const & uniBlocksFile, ToDo toDo)
{
  string uniBlocks;
  try
  {
    ReaderPtr<Reader>(GetPlatform().GetReader(uniBlocksFile)).ReadAsString(uniBlocks);
  }
  catch (RootException const & e)
  {
    LOG(LCRITICAL, ("Error reading uniblock description: ", e.what()));
    return;
  }

  istringstream fin(uniBlocks);
  while (true)
  {
    string name;
    strings::UniChar start;
    strings::UniChar end;
    fin >> name >> std::hex >> start >> std::hex >> end;
    if (!fin)
      break;

    toDo(name, start, end);
  }
}

template <typename ToDo>
void ParseFontList(string const & fontListFile, ToDo toDo)
{
  string fontList;
  try
  {
    ReaderPtr<Reader>(GetPlatform().GetReader(fontListFile)).ReadAsString(fontList);
  }
  catch(RootException const & e)
  {
    LOG(LWARNING, ("Error reading font list ", fontListFile, " : ", e.what()));
    return;
  }

  istringstream fin(fontList);
  while (true)
  {
    string ubName;
    string fontName;
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

  Font(uint32_t sdfScale, ReaderPtr<Reader> fontReader, FT_Library lib)
    : m_fontReader(fontReader)
    , m_fontFace(nullptr)
    , m_sdfScale(sdfScale)
  {
    m_stream.base = 0;
    m_stream.size = m_fontReader.Size();
    m_stream.pos = 0;
    m_stream.descriptor.pointer = &m_fontReader;
    m_stream.pathname.pointer = 0;
    m_stream.read = &Font::Read;
    m_stream.close = &Font::Close;
    m_stream.memory = 0;
    m_stream.cursor = 0;
    m_stream.limit = 0;

    FT_Open_Args args;
    args.flags = FT_OPEN_STREAM;
    args.memory_base = 0;
    args.memory_size = 0;
    args.pathname = 0;
    args.stream = &m_stream;
    args.driver = 0;
    args.num_params = 0;
    args.params = 0;

    FT_Error const err = FT_Open_Face(lib, &args, 0, &m_fontFace);
#ifdef DEBUG
    if (err)
      LOG(LWARNING, ("Freetype:", g_FT_Errors[err].m_code, g_FT_Errors[err].m_message));
#endif
    if (err || !IsValid())
      MYTHROW(InvalidFontException, ());
  }

  bool IsValid() const
  {
    return m_fontFace != nullptr && m_fontFace->num_glyphs > 0;
  }

  void DestroyFont()
  {
    if (m_fontFace != nullptr)
    {
      FREETYPE_CHECK(FT_Done_Face(m_fontFace));
      m_fontFace = nullptr;
    }
  }

  bool HasGlyph(strings::UniChar unicodePoint) const
  {
    return FT_Get_Char_Index(m_fontFace, unicodePoint) != 0;
  }

  GlyphManager::Glyph GetGlyph(strings::UniChar unicodePoint, uint32_t baseHeight) const
  {
    FREETYPE_CHECK(FT_Set_Pixel_Sizes(m_fontFace, m_sdfScale * baseHeight, m_sdfScale * baseHeight));
    FREETYPE_CHECK(FT_Load_Glyph(m_fontFace, FT_Get_Char_Index(m_fontFace, unicodePoint), FT_LOAD_RENDER));

    FT_Glyph glyph;
    FREETYPE_CHECK(FT_Get_Glyph(m_fontFace->glyph, &glyph));

    FT_BBox bbox;
    FT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_PIXELS , &bbox);

    FT_Bitmap bitmap = m_fontFace->glyph->bitmap;

    float const scale = 1.0f / m_sdfScale;

    SharedBufferManager::shared_buffer_ptr_t data;
    int imageWidth = bitmap.width;
    int imageHeight = bitmap.rows;
    if (bitmap.buffer != nullptr)
    {
      sdf_image::SdfImage img(bitmap.rows, bitmap.pitch, bitmap.buffer, m_sdfScale * kSdfBorder);
      imageWidth = img.GetWidth() * scale;
      imageHeight = img.GetHeight() * scale;

      size_t bufferSize = bitmap.rows * bitmap.pitch;
      data = SharedBufferManager::instance().reserveSharedBuffer(bufferSize);
      memcpy(&(*data)[0], bitmap.buffer, bufferSize);
    }

    GlyphManager::Glyph result;
    result.m_image = GlyphManager::GlyphImage
    {
      imageWidth, imageHeight,
      static_cast<int>(bitmap.rows), bitmap.pitch,
      data
    };

    result.m_metrics = GlyphManager::GlyphMetrics
    {
      static_cast<float>(glyph->advance.x >> 16) * scale,
      static_cast<float>(glyph->advance.y >> 16)  * scale,
      static_cast<float>(bbox.xMin) * scale,
      static_cast<float>(bbox.yMin) * scale,
      true
    };

    result.m_code = unicodePoint;

    FT_Done_Glyph(glyph);

    return result;
  }

  GlyphManager::Glyph GenerateGlyph(GlyphManager::Glyph const & glyph) const
  {
    if (glyph.m_image.m_data != nullptr)
    {
      GlyphManager::Glyph resultGlyph;
      resultGlyph.m_metrics = glyph.m_metrics;
      resultGlyph.m_fontIndex = glyph.m_fontIndex;
      resultGlyph.m_code = glyph.m_code;

      sdf_image::SdfImage img(glyph.m_image.m_bitmapRows, glyph.m_image.m_bitmapPitch,
                              glyph.m_image.m_data->data(), m_sdfScale * kSdfBorder);

      img.GenerateSDF(1.0f / (float)m_sdfScale);

      ASSERT(img.GetWidth() == glyph.m_image.m_width, ());
      ASSERT(img.GetHeight() == glyph.m_image.m_height, ());

      size_t bufferSize = my::NextPowOf2(glyph.m_image.m_width * glyph.m_image.m_height);
      resultGlyph.m_image.m_width = glyph.m_image.m_width;
      resultGlyph.m_image.m_height = glyph.m_image.m_height;
      resultGlyph.m_image.m_bitmapRows = 0;
      resultGlyph.m_image.m_bitmapPitch = 0;
      resultGlyph.m_image.m_data = SharedBufferManager::instance().reserveSharedBuffer(bufferSize);

      img.GetData(*resultGlyph.m_image.m_data);
      return resultGlyph;
    }
    return glyph;
  }

  void GetCharcodes(vector<FT_ULong> & charcodes)
  {
    FT_UInt gindex;
    charcodes.push_back(FT_Get_First_Char(m_fontFace, &gindex));
    while (gindex)
      charcodes.push_back(FT_Get_Next_Char(m_fontFace, charcodes.back(), &gindex));

    sort(charcodes.begin(), charcodes.end());
    charcodes.erase(unique(charcodes.begin(), charcodes.end()), charcodes.end());
  }

  static unsigned long Read(FT_Stream stream, unsigned long offset, unsigned char * buffer, unsigned long count)
  {
    if (count != 0)
    {
      ReaderPtr<Reader> * reader = reinterpret_cast<ReaderPtr<Reader> *>(stream->descriptor.pointer);
      reader->Read(offset, buffer, count);
    }

    return count;
  }

  static void Close(FT_Stream){}

  void MarkGlyphReady(strings::UniChar code)
  {
    m_readyGlyphs.insert(code);
  }

  bool IsGlyphReady(strings::UniChar code) const
  {
    return m_readyGlyphs.find(code) != m_readyGlyphs.end();
  }

private:
  ReaderPtr<Reader> m_fontReader;
  FT_StreamRec_ m_stream;
  FT_Face m_fontFace;
  uint32_t m_sdfScale;

  unordered_set<strings::UniChar> m_readyGlyphs;
};

}

/// Information about single unicode block.
struct UnicodeBlock
{
  string m_name;

  strings::UniChar m_start;
  strings::UniChar m_end;
  vector<int> m_fontsWeight;

  UnicodeBlock(string const & name, strings::UniChar start, strings::UniChar end)
    : m_name(name)
    , m_start(start)
    , m_end(end)
  {
  }

  int GetFontOffset(int idx) const
  {
    if (m_fontsWeight.empty())
      return kInvalidFont;

    int maxWeight = 0;
    int upperBoundWeight = numeric_limits<int>::max();
    if (idx != kInvalidFont)
      upperBoundWeight = m_fontsWeight[idx];

    int index = kInvalidFont;
    for (size_t i = 0; i < m_fontsWeight.size(); ++i)
    {
      int w = m_fontsWeight[i];
      if (w < upperBoundWeight && w > maxWeight)
      {
        maxWeight = w;
        index = i;
      }
    }

    return index;
  }

  bool HasSymbol(strings::UniChar sym) const
  {
    return (m_start <= sym) && (m_end >= sym);
  }
};

typedef vector<UnicodeBlock> TUniBlocks;
typedef TUniBlocks::const_iterator TUniBlockIter;

struct GlyphManager::Impl
{
  FT_Library m_library;
  TUniBlocks m_blocks;
  TUniBlockIter m_lastUsedBlock;
  vector<unique_ptr<Font>> m_fonts;

  uint32_t m_baseGlyphHeight;
};

GlyphManager::GlyphManager(GlyphManager::Params const & params)
  : m_impl(new Impl())
{
  m_impl->m_baseGlyphHeight = params.m_baseGlyphHeight;

  typedef pair<string, string> TFontAndBlockName;
  typedef buffer_vector<TFontAndBlockName, 64> TFontLst;

  TFontLst whitelst;
  TFontLst blacklst;

  m_impl->m_blocks.reserve(160);
  ParseUniBlocks(params.m_uniBlocks, [this](string const & name, strings::UniChar start, strings::UniChar end)
  {
    m_impl->m_blocks.push_back(UnicodeBlock(name, start, end));
  });

  ParseFontList(params.m_whitelist, [&whitelst](string const & ubName, string const & fontName)
  {
    whitelst.push_back(TFontAndBlockName(fontName, ubName));
  });

  ParseFontList(params.m_blacklist, [&blacklst](string const & ubName, string const & fontName)
  {
    blacklst.push_back(TFontAndBlockName(fontName, ubName));
  });

  m_impl->m_fonts.reserve(params.m_fonts.size());

  FREETYPE_CHECK(FT_Init_FreeType(&m_impl->m_library));

  for (string const & fontName : params.m_fonts)
  {
    bool ignoreFont = false;
    for_each(blacklst.begin(), blacklst.end(), [&ignoreFont, &fontName](TFontAndBlockName const & p)
    {
      if (p.first == fontName && p.second == "*")
        ignoreFont = true;
    });

    if (ignoreFont)
      continue;

    vector<FT_ULong> charCodes;
    try
    {
      m_impl->m_fonts.emplace_back(make_unique<Font>(params.m_sdfScale, GetPlatform().GetReader(fontName), m_impl->m_library));
      m_impl->m_fonts.back()->GetCharcodes(charCodes);
    }
    catch(RootException const & e)
    {
      LOG(LWARNING, ("Error read font file : ", e.what()));
      continue;
    }

    typedef size_t TBlockIndex;
    typedef int TCharCounter;
    typedef pair<TBlockIndex, TCharCounter> TCoverNode;
    typedef vector<TCoverNode> TCoverInfo;

    size_t currentUniBlock = 0;
    TCoverInfo coverInfo;
    for (FT_ULong const & charCode : charCodes)
    {
      while(currentUniBlock < m_impl->m_blocks.size())
      {
        if (m_impl->m_blocks[currentUniBlock].HasSymbol(charCode))
          break;
        ++currentUniBlock;
      }

      if (currentUniBlock >= m_impl->m_blocks.size())
        break;

      if (coverInfo.empty() || coverInfo.back().first != currentUniBlock)
        coverInfo.push_back(make_pair(currentUniBlock, 1));
      else
        ++coverInfo.back().second;
    }

    typedef function<void (UnicodeBlock const & uniBlock, TCoverNode & node)> TUpdateCoverInfoFn;
    auto enumerateFn = [this, &coverInfo, &fontName] (TFontLst const & lst, TUpdateCoverInfoFn const & fn)
    {
      for (TFontAndBlockName const & b : lst)
      {
        if (b.first != fontName)
          continue;

        for (TCoverNode & node : coverInfo)
        {
          UnicodeBlock const & uniBlock = m_impl->m_blocks[node.first];
          if (uniBlock.m_name == b.second)
          {
            fn(uniBlock, node);
            break;
          }
          else if (b.second == "*")
            fn(uniBlock, node);
        }
      }
    };

    enumerateFn(blacklst, [](UnicodeBlock const &, TCoverNode & node)
    {
      node.second = 0;
    });

    enumerateFn(whitelst, [this](UnicodeBlock const & uniBlock, TCoverNode & node)
    {
      node.second = uniBlock.m_end + 1 - uniBlock.m_start + m_impl->m_fonts.size();
    });

    for (TCoverNode & node : coverInfo)
    {
      UnicodeBlock & uniBlock = m_impl->m_blocks[node.first];
      uniBlock.m_fontsWeight.resize(m_impl->m_fonts.size(), 0);
      uniBlock.m_fontsWeight.back() = node.second;
    }
  }

  m_impl->m_lastUsedBlock = m_impl->m_blocks.end();
}

GlyphManager::~GlyphManager()
{
  for (auto const & f : m_impl->m_fonts)
    f->DestroyFont();

  FREETYPE_CHECK(FT_Done_FreeType(m_impl->m_library));
  delete m_impl;
}

int GlyphManager::GetFontIndex(strings::UniChar unicodePoint)
{
  TUniBlockIter iter = m_impl->m_blocks.end();
  if (m_impl->m_lastUsedBlock != m_impl->m_blocks.end() && m_impl->m_lastUsedBlock->HasSymbol(unicodePoint))
  {
    iter = m_impl->m_lastUsedBlock;
  }
  else
  {
    if (iter == m_impl->m_blocks.end() || !iter->HasSymbol(unicodePoint))
    {
      iter = lower_bound(m_impl->m_blocks.begin(), m_impl->m_blocks.end(), unicodePoint,
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
  TUniBlockIter iter = lower_bound(m_impl->m_blocks.begin(), m_impl->m_blocks.end(), unicodePoint,
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
  for (int fontIndex = block.GetFontOffset(kInvalidFont); fontIndex != kInvalidFont; fontIndex = block.GetFontOffset(fontIndex))
  {
    ASSERT_LESS(fontIndex, m_impl->m_fonts.size(), ());
    auto const & f = m_impl->m_fonts[fontIndex];
    if (f->HasGlyph(unicodePoint))
      return fontIndex;
  }

  return kInvalidFont;
}

GlyphManager::Glyph GlyphManager::GetGlyph(strings::UniChar unicodePoint)
{
  int const fontIndex = GetFontIndex(unicodePoint);
  if (fontIndex == kInvalidFont)
    return GetInvalidGlyph();

  auto const & f = m_impl->m_fonts[fontIndex];
  Glyph glyph = f->GetGlyph(unicodePoint, m_impl->m_baseGlyphHeight);
  glyph.m_fontIndex = fontIndex;
  return glyph;
}

GlyphManager::Glyph GlyphManager::GenerateGlyph(Glyph const & glyph) const
{
  ASSERT_NOT_EQUAL(glyph.m_fontIndex, -1, ());
  ASSERT_LESS(glyph.m_fontIndex, m_impl->m_fonts.size(), ());
  auto const & f = m_impl->m_fonts[glyph.m_fontIndex];
  return f->GenerateGlyph(glyph);
}

void GlyphManager::ForEachUnicodeBlock(GlyphManager::TUniBlockCallback const & fn) const
{
  for (UnicodeBlock const & uni : m_impl->m_blocks)
    fn(uni.m_start, uni.m_end);
}

void GlyphManager::MarkGlyphReady(Glyph const & glyph)
{
  ASSERT_GREATER_OR_EQUAL(glyph.m_fontIndex, 0, ());
  ASSERT_LESS(glyph.m_fontIndex, m_impl->m_fonts.size(), ());
  m_impl->m_fonts[glyph.m_fontIndex]->MarkGlyphReady(glyph.m_code);
}

bool GlyphManager::AreGlyphsReady(strings::UniString const & str) const
{
  for (auto const & code : str)
  {
    int const fontIndex = GetFontIndexImmutable(code);
    if (fontIndex == kInvalidFont)
      continue;

    if (!m_impl->m_fonts[fontIndex]->IsGlyphReady(code))
      return false;
  }

  return true;
}

GlyphManager::Glyph GlyphManager::GetInvalidGlyph() const
{
  strings::UniChar const kInvalidGlyphCode = 0x9;
  int const kFontId = 0;

  static bool s_inited = false;
  static Glyph s_glyph;

  if (!s_inited)
  {
    ASSERT(!m_impl->m_fonts.empty(), ());
    s_glyph = m_impl->m_fonts[kFontId]->GetGlyph(kInvalidGlyphCode, m_impl->m_baseGlyphHeight);
    s_glyph.m_metrics.m_isValid = false;
    s_glyph.m_fontIndex = kFontId;
    s_glyph.m_code = kInvalidGlyphCode;
    s_inited = true;
  }

  return s_glyph;
}

}
