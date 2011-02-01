#include "../base/SRC_FIRST.hpp"
#include "glyph_cache_impl.hpp"

#include <../cache/ftcglyph.h>
#include <../cache/ftcimage.h>
#include <../cache/ftcsbits.h>
#include <../cache/ftccback.h>
#include <../cache/ftccache.h>

#include "../std/fstream.hpp"
#include "../base/ptr_utils.hpp"


namespace yg
{
  UnicodeBlock::UnicodeBlock(string const & name, uint32_t start, uint32_t end)
    : m_start(start), m_end(end), m_name(name)
  {}

  bool UnicodeBlock::hasSymbol(uint16_t sym) const
  {
    return (m_start <= sym) && (m_end >= sym);
  }

  Font::Font(char const * name) : m_name(name), m_fontData(name, true)
  {
  }

  FT_Error Font::CreateFaceID(FT_Library library, FT_Face *face)
  {
    return FT_New_Memory_Face(library, (unsigned char*)m_fontData.data(), m_fontData.size(), 0, face);
  }

  void GlyphCacheImpl::initBlocks(string const & fileName)
  {
    ifstream fin(fileName.c_str());
    while (true)
    {
      string name;
      uint16_t start;
      uint16_t end;
      fin >> name >> std::hex >> start >> std::hex >> end;
      if (!fin)
        break;

      m_unicodeBlocks.push_back(UnicodeBlock(name, start, end));
    }

    m_lastUsedBlock = m_unicodeBlocks.end();
  }

  bool sym_in_block(UnicodeBlock & b, uint16_t sym)
  {
    return (b.m_start <= sym);
  }

  bool greater_coverage(pair<int, shared_ptr<Font> > const & l, pair<int, shared_ptr<Font> > const & r)
  {
    return l.first > r.first;
  }

  void GlyphCacheImpl::addFonts(vector<string> const & fontNames)
  {
    for (int i = 0; i < fontNames.size(); ++i)
      addFont(fontNames[i].c_str());

    for (unicode_blocks_t::const_iterator it = m_unicodeBlocks.begin(); it != m_unicodeBlocks.end(); ++it)
    {
      if (it->m_fonts.empty())
        LOG(LINFO, (it->m_name, " unicode block is empty"));
    }
  }

  void GlyphCacheImpl::addFont(char const * fileName)
  {
    m_fonts.push_back(make_shared_ptr(new Font(fileName)));

    /// obtaining all glyphs, supported by this font
    FT_Face face;
    m_fonts.back()->CreateFaceID(m_lib, &face);

    vector<FT_ULong> charcodes;

    FT_UInt gindex;
    charcodes.push_back(FT_Get_First_Char(face, &gindex));
    while (gindex)
      charcodes.push_back(FT_Get_Next_Char(face, charcodes.back(), &gindex));

    sort(charcodes.begin(), charcodes.end());
    unique(charcodes.begin(), charcodes.end());

    FT_Done_Face(face);

    /// modifying the m_unicodeBlocks

    int lastUBEnd = 0;

    unicode_blocks_t::iterator ubIt = m_unicodeBlocks.begin();
    vector<FT_ULong>::iterator ccIt = charcodes.begin();

    while (true)
    {
      while (ubIt != m_unicodeBlocks.end())
      {
        if ((*ccIt > lastUBEnd) && (*ccIt < ubIt->m_start))
          LOG(LINFO, ("Symbol with code ", (uint16_t)*ccIt, "lies between two unicode blocks!"));
        if (ubIt->hasSymbol(*ccIt))
          break;
        lastUBEnd = ubIt->m_end;
        ++ubIt;
      }

      if (ubIt == m_unicodeBlocks.end())
        break;

      /// here we have unicode block, which contains the specified symbol.
      if (ubIt->m_fonts.empty() || (ubIt->m_fonts.back() != m_fonts.back()))
      {
        ubIt->m_fonts.push_back(m_fonts.back());
        ubIt->m_coverage.push_back(0);
      }

      ++ubIt->m_coverage.back();
      ++ccIt;
    }

    /// rearrange fonts in all unicode blocks according to it's coverage
    for (ubIt = m_unicodeBlocks.begin(); ubIt != m_unicodeBlocks.end(); ++ubIt)
    {
      vector<pair<int, shared_ptr<Font> > > sortData;

      for (unsigned i = 0; i < ubIt->m_fonts.size(); ++i)
        sortData.push_back(make_pair<int, shared_ptr<Font> >(ubIt->m_coverage[i], ubIt->m_fonts[i]));

      sort(sortData.begin(), sortData.end(), &greater_coverage);

      for (unsigned i = 0; i < ubIt->m_fonts.size(); ++i)
      {
        ubIt->m_coverage[i] = sortData[i].first;
        ubIt->m_fonts[i] = sortData[i].second;
      }
    }

  }

  vector<shared_ptr<Font> > & GlyphCacheImpl::getFonts(uint16_t sym)
  {
    if ((m_lastUsedBlock != m_unicodeBlocks.end()) && (m_lastUsedBlock->hasSymbol(sym)))
     return m_lastUsedBlock->m_fonts;

    unicode_blocks_t::iterator it = lower_bound(m_unicodeBlocks.begin(),
                                                m_unicodeBlocks.end(),
                                                sym,
                                                &sym_in_block);

    if (it == m_unicodeBlocks.end())
     it = (--m_unicodeBlocks.rbegin()).base();
    else
      if (it != m_unicodeBlocks.begin())
        it = --it;

    m_lastUsedBlock = it;

    if ((it != m_unicodeBlocks.end()) && (it->hasSymbol(sym)))
    {
      if (it->m_fonts.empty())
        it->m_fonts.push_back(m_fonts.front());

      return it->m_fonts;
    }
    else
      return m_fonts;
  }


  GlyphCacheImpl::GlyphCacheImpl(std::string const & blocksFileName, size_t maxSize)
  {
    initBlocks(blocksFileName);

    FTCHECK(FT_Init_FreeType(&m_lib));

    /// Initializing caches
    FTCHECK(FTC_Manager_New(m_lib, 3, 10, maxSize, &RequestFace, 0, &m_manager));

    FTCHECK(FTC_ImageCache_New(m_manager, &m_normalGlyphCache));
    FTCHECK(FTC_ImageCache_New(m_manager, &m_glyphMetricsCache));

    /// Initializing stroker
    FTCHECK(FT_Stroker_New(m_lib, &m_stroker));
    FT_Stroker_Set(m_stroker, 2 * 64, FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);

    FTCHECK(FTC_StrokedImageCache_New(m_manager, &m_strokedGlyphCache, m_stroker));

    FTCHECK(FTC_CMapCache_New(m_manager, &m_charMapCache));
  }

  GlyphCacheImpl::~GlyphCacheImpl()
  {
    FTC_Manager_Done(m_manager);
    FT_Stroker_Done(m_stroker);
    FT_Done_FreeType(m_lib);
  }

  FT_Error GlyphCacheImpl::RequestFace(FTC_FaceID faceID, FT_Library library, FT_Pointer /*requestData*/, FT_Face * face)
  {
    //GlyphCacheImpl * glyphCacheImpl = reinterpret_cast<GlyphCacheImpl*>(requestData);
    Font * font = reinterpret_cast<Font*>(faceID);
    return font->CreateFaceID(library, face);
  }
}
