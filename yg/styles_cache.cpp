#include "../base/SRC_FIRST.hpp"

#include "styles_cache.hpp"
#include "text_element.hpp"
#include "glyph_cache.hpp"
#include "skin_page.hpp"
#include "resource_manager.hpp"

namespace yg
{
  StylesCache::StylesCache(shared_ptr<ResourceManager> const & rm,
                           int glyphCacheID,
                           int maxPagesCount)
    : m_rm(rm),
      m_maxPagesCount(maxPagesCount)
  {
    m_glyphCache = m_rm->glyphCache(glyphCacheID);
  }

  StylesCache::~StylesCache()
  {
    for (unsigned i = 0; i < m_cachePages.size(); ++i)
      m_cachePages[i]->freeTexture();
  }

  vector<shared_ptr<SkinPage> > & StylesCache::cachePages()
  {
    return m_cachePages;
  }

  shared_ptr<ResourceManager> const & StylesCache::resourceManager()
  {
    return m_rm;
  }

  GlyphCache * StylesCache::glyphCache()
  {
    return m_glyphCache;
  }

  void StylesCache::upload()
  {
    for (unsigned i = 0; i < m_cachePages.size(); ++i)
      m_cachePages[i]->uploadData();
  }
}
