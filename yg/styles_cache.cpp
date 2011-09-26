#include "../base/SRC_FIRST.hpp"

#include "styles_cache.hpp"
#include "text_element.hpp"
#include "glyph_cache.hpp"
#include "skin_page.hpp"
#include "resource_manager.hpp"
#include "base_texture.hpp"
#include "internal/opengl.hpp"

#include "../base/thread.hpp"

namespace yg
{
  StylesCache::StylesCache(shared_ptr<ResourceManager> const & rm,
                           int glyphCacheID)
    : m_rm(rm)
  {
    m_glyphCache = m_rm->glyphCache(glyphCacheID);
    m_cachePage.reset(new SkinPage());
    m_cachePage->setTexture(m_rm->styleCacheTextures()->Reserve());
  }

  StylesCache::~StylesCache()
  {
    m_rm->styleCacheTextures()->Free(m_cachePage->texture());
  }

  shared_ptr<SkinPage> const & StylesCache::cachePage()
  {
    return m_cachePage;
  }

  shared_ptr<ResourceManager> const & StylesCache::resourceManager()
  {
    return m_rm;
  }

  GlyphCache * StylesCache::glyphCache()
  {
    return m_glyphCache;
  }

  void StylesCache::clear()
  {
    m_cachePage->clear();
  }

  void StylesCache::upload()
  {
    m_cachePage->uploadData();
    OGLCHECK(glFinish());
  }

  bool StylesCache::hasRoom(m2::PointU const * sizes, size_t cnt) const
  {
    return m_cachePage->hasRoom(sizes, cnt);
  }
}
