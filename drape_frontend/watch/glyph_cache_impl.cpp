#include "drape_frontend/watch/glyph_cache_impl.hpp"

#include "platform/platform.hpp"

#include "coding/reader.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "std/bind.hpp"
#include "std/cstring.hpp"

#include <freetype/ftcache.h>

#define FREETYPE_CHECK(x) x
#define FREETYPE_CHECK_RETURN(x, msg) FREETYPE_CHECK(x)

namespace df
{
namespace watch
{

UnicodeBlock::UnicodeBlock(string const & name, strings::UniChar start, strings::UniChar end)
  : m_name(name), m_start(start), m_end(end)
{}

bool UnicodeBlock::hasSymbol(strings::UniChar sym) const
{
  return (m_start <= sym) && (m_end >= sym);
}

/// Called by FreeType to read data
static unsigned long FTStreamIOFunc(FT_Stream stream,
                                    unsigned long offset,
                                    unsigned char * buffer,
                                    unsigned long count)
{
  // FreeType can call us with 0 to "Skip" bytes
  if (count != 0)
    reinterpret_cast<ReaderPtr<Reader> *>(stream->descriptor.pointer)->Read(offset, buffer, count);
  return count;
}

static void FTStreamCloseFunc(FT_Stream)
{
}

Font::Font(ReaderPtr<Reader> const & fontReader) : m_fontReader(fontReader)
{
  m_fontStream = new FT_StreamRec;
  m_fontStream->base = 0;
  m_fontStream->size = m_fontReader.Size();
  m_fontStream->pos = 0;
  m_fontStream->descriptor.pointer = &m_fontReader;
  m_fontStream->pathname.pointer = 0;
  m_fontStream->read = &FTStreamIOFunc;
  m_fontStream->close = &FTStreamCloseFunc;
  m_fontStream->memory = 0;
  m_fontStream->cursor = 0;
  m_fontStream->limit = 0;
}

Font::~Font()
{
  delete m_fontStream;
}

FT_Error Font::CreateFaceID(FT_Library library, FT_Face * face)
{
  FT_Open_Args args;
  args.flags = FT_OPEN_STREAM;
  args.memory_base = 0;
  args.memory_size = 0;
  args.pathname = 0;
  args.stream = m_fontStream;
  args.driver = 0;
  args.num_params = 0;
  args.params = 0;
  return FT_Open_Face(library, &args, 0, face);
  //return FT_New_Memory_Face(library, (unsigned char*)m_fontData.data(), m_fontData.size(), 0, face);
}

void GlyphCacheImpl::initBlocks(string const & fileName)
{
  string buffer;
  try
  {
    ReaderPtr<Reader>(GetPlatform().GetReader(fileName)).ReadAsString(buffer);
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("Error reading unicode blocks: ", e.what()));
    return;
  }

  istringstream fin(buffer);
  while (true)
  {
    string name;
    strings::UniChar start;
    strings::UniChar end;
    fin >> name >> std::hex >> start >> std::hex >> end;
    if (!fin)
      break;

    m_unicodeBlocks.push_back(UnicodeBlock(name, start, end));
  }

  m_lastUsedBlock = m_unicodeBlocks.end();
}

bool find_ub_by_name(string const & ubName, UnicodeBlock const & ub)
{
  return ubName == ub.m_name;
}

void GlyphCacheImpl::initFonts(string const & whiteListFile, string const & blackListFile)
{
  {
    string buffer;
    try
    {
      ReaderPtr<Reader>(GetPlatform().GetReader(whiteListFile)).ReadAsString(buffer);
    }
    catch (RootException const & e)
    {
      LOG(LERROR, ("Error reading white list fonts: ", e.what()));
      return;
    }

    istringstream fin(buffer);
    while (true)
    {
      string ubName;
      string fontName;
      fin >> ubName >> fontName;
      if (!fin)
        break;

      LOG(LDEBUG, ("whitelisting ", fontName, " for ", ubName));

      if (ubName == "*")
        for (unicode_blocks_t::iterator it = m_unicodeBlocks.begin(); it != m_unicodeBlocks.end(); ++it)
          it->m_whitelist.push_back(fontName);
      else
      {
        unicode_blocks_t::iterator it = find_if(m_unicodeBlocks.begin(), m_unicodeBlocks.end(), bind(&find_ub_by_name, ubName, _1));
        if (it != m_unicodeBlocks.end())
          it->m_whitelist.push_back(fontName);
      }
    }
  }

  {
    string buffer;
    try
    {
      ReaderPtr<Reader>(GetPlatform().GetReader(blackListFile)).ReadAsString(buffer);
    }
    catch (RootException const & e)
    {
      LOG(LERROR, ("Error reading black list fonts: ", e.what()));
      return;
    }

    istringstream fin(buffer);
    while (true)
    {
      string ubName;
      string fontName;
      fin >> ubName >> fontName;
      if (!fin)
        break;

      LOG(LDEBUG, ("blacklisting ", fontName, " for ", ubName));

      if (ubName == "*")
        for (unicode_blocks_t::iterator it = m_unicodeBlocks.begin(); it != m_unicodeBlocks.end(); ++it)
          it->m_blacklist.push_back(fontName);
      else
      {
        unicode_blocks_t::iterator it = find_if(m_unicodeBlocks.begin(), m_unicodeBlocks.end(), bind(&find_ub_by_name, ubName, _1));
        if (it != m_unicodeBlocks.end())
          it->m_blacklist.push_back(fontName);
      }
    }
  }
}

bool greater_coverage(pair<int, shared_ptr<Font> > const & l, pair<int, shared_ptr<Font> > const & r)
{
  return l.first > r.first;
}

void GlyphCacheImpl::addFonts(vector<string> const & fontNames)
{
  if (m_isDebugging)
    return;

  for (size_t i = 0; i < fontNames.size(); ++i)
  {
    try
    {
      addFont(fontNames[i].c_str());
    }
    catch (RootException const & ex)
    {
      LOG(LERROR, ("Can't load font", fontNames[i], ex.Msg()));
    }
  }
}

void GlyphCacheImpl::addFont(char const * fileName)
{
  if (m_isDebugging)
    return;

  shared_ptr<Font> pFont(new Font(GetPlatform().GetReader(fileName)));

  // Obtaining all glyphs, supported by this font. Call to FTCHECKRETURN functions may return
  // from routine, so add font to fonts array only in the end.

  FT_Face face;
  FREETYPE_CHECK_RETURN(pFont->CreateFaceID(m_lib, &face), fileName);

  vector<FT_ULong> charcodes;

  FT_UInt gindex;
  charcodes.push_back(FT_Get_First_Char(face, &gindex));
  while (gindex)
    charcodes.push_back(FT_Get_Next_Char(face, charcodes.back(), &gindex));

  sort(charcodes.begin(), charcodes.end());
  charcodes.erase(unique(charcodes.begin(), charcodes.end()), charcodes.end());

  FREETYPE_CHECK_RETURN(FT_Done_Face(face), fileName);

  m_fonts.push_back(pFont);

  // modifying the m_unicodeBlocks
  unicode_blocks_t::iterator ubIt = m_unicodeBlocks.begin();
  vector<FT_ULong>::iterator ccIt = charcodes.begin();

  while (ccIt != charcodes.end())
  {
    while (ubIt != m_unicodeBlocks.end())
    {
      ASSERT ( ccIt != charcodes.end(), () );
      if (ubIt->hasSymbol(*ccIt))
        break;
      ++ubIt;
    }

    if (ubIt == m_unicodeBlocks.end())
      break;

    // here we have unicode block, which contains the specified symbol.
    if (ubIt->m_fonts.empty() || (ubIt->m_fonts.back() != m_fonts.back()))
    {
      ubIt->m_fonts.push_back(m_fonts.back());
      ubIt->m_coverage.push_back(0);

      // checking blacklist and whitelist

      for (size_t i = 0; i < ubIt->m_blacklist.size(); ++i)
        if (ubIt->m_blacklist[i] == fileName)
        {
          // if font is blacklisted for this unicode block
          ubIt->m_coverage.back() = -1;
        }

      for (size_t i = 0; i < ubIt->m_whitelist.size(); ++i)
        if (ubIt->m_whitelist[i] == fileName)
        {
          if (ubIt->m_coverage.back() == -1)
          {
            LOG(LWARNING, ("font ", fileName, "is present both at blacklist and whitelist. whitelist prevails."));
          }
          // weight used for sorting are boosted to the top.
          // the order of elements are saved by adding 'i' value as a shift.
          ubIt->m_coverage.back() = ubIt->m_end + 1 - ubIt->m_start + i + 1;
        }
    }

    if ((ubIt->m_coverage.back() >= 0) && (ubIt->m_coverage.back() < ubIt->m_end + 1 - ubIt->m_start))
      ++ubIt->m_coverage.back();
    ++ccIt;
  }

  // rearrange fonts in all unicode blocks according to it's coverage
  for (ubIt = m_unicodeBlocks.begin(); ubIt != m_unicodeBlocks.end(); ++ubIt)
  {
    /// @todo Make sorting of 2 vectors ubIt->m_coverage, ubIt->m_fonts
    /// with one criteria without temporary vector sortData.

    size_t const count = ubIt->m_fonts.size();

    vector<pair<int, shared_ptr<Font> > > sortData;
    sortData.reserve(count);

    for (size_t i = 0; i < count; ++i)
      sortData.push_back(make_pair(ubIt->m_coverage[i], ubIt->m_fonts[i]));

    sort(sortData.begin(), sortData.end(), &greater_coverage);

    for (size_t i = 0; i < count; ++i)
    {
      ubIt->m_coverage[i] = sortData[i].first;
      ubIt->m_fonts[i] = sortData[i].second;
    }
  }
}

struct sym_in_block
{
  bool operator() (UnicodeBlock const & b, strings::UniChar sym) const
  {
    return (b.m_start < sym);
  }
  bool operator() (strings::UniChar sym, UnicodeBlock const & b) const
  {
    return (sym < b.m_start);
  }
  bool operator() (UnicodeBlock const & b1, UnicodeBlock const & b2) const
  {
    return (b1.m_start < b2.m_start);
  }
};

vector<shared_ptr<Font> > & GlyphCacheImpl::getFonts(strings::UniChar sym)
{
  if ((m_lastUsedBlock != m_unicodeBlocks.end()) && m_lastUsedBlock->hasSymbol(sym))
   return m_lastUsedBlock->m_fonts;

  unicode_blocks_t::iterator it = lower_bound(m_unicodeBlocks.begin(),
                                              m_unicodeBlocks.end(),
                                              sym,
                                              sym_in_block());

  if (it == m_unicodeBlocks.end())
   it = m_unicodeBlocks.end()-1;
  else
    if (it != m_unicodeBlocks.begin())
      it = it-1;

  m_lastUsedBlock = it;

  if ((it != m_unicodeBlocks.end()) && it->hasSymbol(sym))
  {
    if (it->m_fonts.empty())
    {
      LOG(LDEBUG, ("querying symbol for empty ", it->m_name, " unicode block"));
      ASSERT(!m_fonts.empty(), ("Empty fonts container"));

      it->m_fonts.push_back(m_fonts.front());
    }

    return it->m_fonts;
  }
  else
    return m_fonts;
}


GlyphCacheImpl::GlyphCacheImpl(GlyphCache::Params const & params)
{
  m_isDebugging = params.m_isDebugging;

  initBlocks(params.m_blocksFile);
  initFonts(params.m_whiteListFile, params.m_blackListFile);

  if (!m_isDebugging)
  {
    FREETYPE_CHECK(FT_Init_FreeType(&m_lib));

    /// Initializing caches
    FREETYPE_CHECK(FTC_Manager_New(m_lib, 3, 10, params.m_maxSize, &RequestFace, 0, &m_manager));

    FREETYPE_CHECK(FTC_ImageCache_New(m_manager, &m_normalGlyphCache));
    FREETYPE_CHECK(FTC_StrokedImageCache_New(m_manager, &m_strokedGlyphCache));

    FREETYPE_CHECK(FTC_ImageCache_New(m_manager, &m_normalMetricsCache));
    FREETYPE_CHECK(FTC_StrokedImageCache_New(m_manager, &m_strokedMetricsCache));

    /// Initializing stroker
    FREETYPE_CHECK(FT_Stroker_New(m_lib, &m_stroker));
    FT_Stroker_Set(m_stroker, FT_Fixed(params.m_visualScale * 2 * 64), FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);

    FREETYPE_CHECK(FTC_CMapCache_New(m_manager, &m_charMapCache));
  }
  else
  {
    /// initialize fake bitmap
  }
}

GlyphCacheImpl::~GlyphCacheImpl()
{
  if (!m_isDebugging)
  {
    FTC_Manager_Done(m_manager);
    FT_Stroker_Done(m_stroker);
    FT_Done_FreeType(m_lib);
  }
}

int GlyphCacheImpl::getCharIDX(shared_ptr<Font> const & font, strings::UniChar symbolCode)
{
  if (m_isDebugging)
    return 0;

  FTC_FaceID faceID = reinterpret_cast<FTC_FaceID>(font.get());

  return FTC_CMapCache_Lookup(
      m_charMapCache,
      faceID,
      -1,
      symbolCode
      );
}

pair<Font*, int> const GlyphCacheImpl::getCharIDX(GlyphKey const & key)
{
  if (m_isDebugging)
    return make_pair((Font*)0, 0);

  vector<shared_ptr<Font> > & fonts = getFonts(key.m_symbolCode);

  Font * font = 0;

  int charIDX;

  for (size_t i = 0; i < fonts.size(); ++i)
  {
    charIDX = getCharIDX(fonts[i], key.m_symbolCode);

    if (charIDX != 0)
      return make_pair(fonts[i].get(), charIDX);
  }

#ifdef DEBUG

  for (size_t i = 0; i < m_unicodeBlocks.size(); ++i)
  {
    if (m_unicodeBlocks[i].hasSymbol(key.m_symbolCode))
    {
      LOG(LDEBUG, ("Symbol", key.m_symbolCode, "not found, unicodeBlock=", m_unicodeBlocks[i].m_name));
      break;
    }
  }

#endif

  font = fonts.front().get();

  /// taking substitution character from the first font in the list
  charIDX = getCharIDX(fonts.front(), 65533);
  if (charIDX == 0)
    charIDX = getCharIDX(fonts.front(), 32);

  return make_pair(font, charIDX);
}

GlyphMetrics const GlyphCacheImpl::getGlyphMetrics(GlyphKey const & key)
{
  pair<Font*, int> charIDX = getCharIDX(key);

  FTC_ScalerRec fontScaler =
  {
    static_cast<FTC_FaceID>(charIDX.first),
    static_cast<FT_UInt>(key.m_fontSize),
    static_cast<FT_UInt>(key.m_fontSize),
    1,
    0,
    0
  };

  FT_Glyph glyph = 0;

  if (key.m_isMask)
  {
    FREETYPE_CHECK(FTC_StrokedImageCache_LookupScaler(
        m_strokedMetricsCache,
        &fontScaler,
        m_stroker,
        FT_LOAD_DEFAULT,
        charIDX.second,
        &glyph,
        0));
  }
  else
  {
    FREETYPE_CHECK(FTC_ImageCache_LookupScaler(
      m_normalMetricsCache,
      &fontScaler,
      FT_LOAD_DEFAULT,
      charIDX.second,
      &glyph,
      0));
  }

  FT_BBox cbox;
  FT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_PIXELS, &cbox);

  GlyphMetrics m =
  {
    static_cast<int>(glyph->advance.x >> 16),
    static_cast<int>(glyph->advance.y >> 16),
    static_cast<int>(cbox.xMin),
    static_cast<int>(cbox.yMin),
    static_cast<int>(cbox.xMax - cbox.xMin),
    static_cast<int>(cbox.yMax - cbox.yMin)
  };

  return m;
}

shared_ptr<GlyphBitmap> const GlyphCacheImpl::getGlyphBitmap(GlyphKey const & key)
{
  pair<Font *, int> charIDX = getCharIDX(key);

  FTC_ScalerRec fontScaler =
  {
    static_cast<FTC_FaceID>(charIDX.first),
    static_cast<FT_UInt>(key.m_fontSize),
    static_cast<FT_UInt>(key.m_fontSize),
    1,
    0,
    0
  };

  FT_Glyph glyph = 0;
  FTC_Node node;

  if (key.m_isMask)
  {
    FREETYPE_CHECK(FTC_StrokedImageCache_LookupScaler(
        m_strokedGlyphCache,
        &fontScaler,
        m_stroker,
        FT_LOAD_DEFAULT,
        charIDX.second,
        &glyph,
        &node
        ));
  }
  else
  {
    FREETYPE_CHECK(FTC_ImageCache_LookupScaler(
        m_normalGlyphCache,
        &fontScaler,
        FT_LOAD_DEFAULT | FT_LOAD_RENDER,
        charIDX.second,
        &glyph,
        &node
        ));
  }

  GlyphBitmap * bitmap = new GlyphBitmap();

  FT_BitmapGlyph bitmapGlyph = (FT_BitmapGlyph)glyph;

  bitmap->m_width = bitmapGlyph ? bitmapGlyph->bitmap.width : 0;
  bitmap->m_height = bitmapGlyph ? bitmapGlyph->bitmap.rows : 0;
  bitmap->m_pitch = bitmapGlyph ? bitmapGlyph->bitmap.pitch : 0;

  if (bitmap->m_width && bitmap->m_height)
  {
    bitmap->m_data.resize(bitmap->m_pitch * bitmap->m_height);
    memcpy(&bitmap->m_data[0],
           bitmapGlyph->bitmap.buffer,
           bitmap->m_pitch * bitmap->m_height);
  }

  FTC_Node_Unref(node, m_manager);

  return shared_ptr<GlyphBitmap>(bitmap);
}

FT_Error GlyphCacheImpl::RequestFace(FTC_FaceID faceID, FT_Library library, FT_Pointer /*requestData*/, FT_Face * face)
{
  //GlyphCacheImpl * glyphCacheImpl = reinterpret_cast<GlyphCacheImpl*>(requestData);
  Font * font = reinterpret_cast<Font*>(faceID);
  return font->CreateFaceID(library, face);
}

}
}
