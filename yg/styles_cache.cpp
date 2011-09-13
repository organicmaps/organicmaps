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

  void StylesCache::cachePathText(PathTextElement const & pte)
  {
    pte.cache(m_cachePages, m_rm, m_glyphCache);
  }

  void StylesCache::cacheStraightText(StraightTextElement const & ste)
  {
    ste.cache(m_cachePages, m_rm, m_glyphCache);
  }

  vector<shared_ptr<SkinPage> > const & StylesCache::cachePages() const
  {
    return m_cachePages;
  }

  void StylesCache::upload()
  {
    for (unsigned i = 0; i < m_cachePages.size(); ++i)
      m_cachePages[i]->uploadData();
  }
}
