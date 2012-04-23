#include "../base/SRC_FIRST.hpp"

#include "resource_style_cache.hpp"
#include "text_element.hpp"
#include "glyph_cache.hpp"
#include "skin_page.hpp"
#include "resource_manager.hpp"
#include "base_texture.hpp"
#include "packets_queue.hpp"
#include "internal/opengl.hpp"
#include "renderer.hpp"

#include "../base/thread.hpp"

namespace yg
{

  struct ResourceStyleCacheContext::Impl
  {
    /// the following sets are used to see, whether
    /// in the current InfoLayer::cache process we've already
    /// caching some resource (glyph, color, penInfo, e.t.c.)
    /// to avoid caching it twice.
    /// @{

    typedef set<PenInfo> TPenInfoSet;
    TPenInfoSet m_penInfoSet;

    typedef set<CircleInfo> TCircleInfoSet;
    TCircleInfoSet m_circleInfoSet;

    typedef set<Color> TColorSet;
    TColorSet m_colorSet;

    typedef set<GlyphKey> TGlyphSet;
    TGlyphSet m_glyphSet;

    /// @}
  };

  ResourceStyleCacheContext::ResourceStyleCacheContext()
  {
    m_impl = new Impl();
  }

  ResourceStyleCacheContext::~ResourceStyleCacheContext()
  {
    delete m_impl;
  }

  bool ResourceStyleCacheContext::hasColor(Color const & c) const
  {
    return m_impl->m_colorSet.count(c);
  }

  void ResourceStyleCacheContext::addColor(Color const & c)
  {
    m_impl->m_colorSet.insert(c);
  }

  bool ResourceStyleCacheContext::hasPenInfo(PenInfo const & pi) const
  {
    return m_impl->m_penInfoSet.count(pi);
  }

  void ResourceStyleCacheContext::addPenInfo(PenInfo const & pi)
  {
    m_impl->m_penInfoSet.insert(pi);
  }

  bool ResourceStyleCacheContext::hasCircleInfo(CircleInfo const & ci) const
  {
    return m_impl->m_circleInfoSet.count(ci);
  }

  void ResourceStyleCacheContext::addCircleInfo(CircleInfo const & ci)
  {
    m_impl->m_circleInfoSet.insert(ci);
  }

  bool ResourceStyleCacheContext::hasGlyph(GlyphKey const & gk) const
  {
    return m_impl->m_glyphSet.count(gk);
  }

  void ResourceStyleCacheContext::addGlyph(GlyphKey const & gk)
  {
    m_impl->m_glyphSet.insert(gk);
  }

  void ResourceStyleCacheContext::clear()
  {
    m_impl->m_circleInfoSet.clear();
    m_impl->m_colorSet.clear();
    m_impl->m_glyphSet.clear();
    m_impl->m_penInfoSet.clear();
  }
  ResourceStyleCache::ResourceStyleCache(shared_ptr<ResourceManager> const & rm,
                                         int glyphCacheID,
                                         yg::gl::PacketsQueue * glQueue)
    : m_rm(rm),
      m_glQueue(glQueue)
  {
    m_glyphCache = m_rm->glyphCache(glyphCacheID);
    m_cachePage.reset(new SkinPage());
    m_cachePage->setTexture(m_rm->styleCacheTextures()->Reserve());
  }

  ResourceStyleCache::~ResourceStyleCache()
  {
    m_rm->styleCacheTextures()->Free(m_cachePage->texture());
  }

  shared_ptr<SkinPage> const & ResourceStyleCache::cachePage()
  {
    return m_cachePage;
  }

  shared_ptr<ResourceManager> const & ResourceStyleCache::resourceManager()
  {
    return m_rm;
  }

  GlyphCache * ResourceStyleCache::glyphCache()
  {
    return m_glyphCache;
  }

  void ResourceStyleCache::clear()
  {
    m_cachePage->clear();
  }

  void ResourceStyleCache::upload()
  {
//    m_cachePage->uploadData(m_glQueue);

    if (m_glQueue)
      m_glQueue->processPacket(yg::gl::Packet(make_shared_ptr(new yg::gl::Renderer::FinishCommand()), yg::gl::Packet::ECommand));

    /// waiting for upload to complete
    if (m_glQueue)
      m_glQueue->completeCommands();
    else
      OGLCHECK(glFinish());
  }

  bool ResourceStyleCache::hasRoom(m2::PointU const * sizes, size_t cnt) const
  {
    return m_cachePage->hasRoom(sizes, cnt);
  }
}
