#include "internal/opengl.hpp"
#include "base_texture.hpp"
#include "data_formats.hpp"
#include "resource_manager.hpp"
#include "skin_loader.hpp"
#include "storage.hpp"
#include "texture.hpp"
#include "vertexbuffer.hpp"
#include "indexbuffer.hpp"

#include "../coding/file_reader.hpp"
#include "../coding/parse_xml.hpp"

#include "../base/logging.hpp"
#include "../base/exception.hpp"

namespace yg
{
  typedef gl::Texture<DATA_TRAITS, true> TDynamicTexture;
  typedef gl::Texture<DATA_TRAITS, false> TStaticTexture;

  DECLARE_EXCEPTION(ResourceManagerException, RootException);

  TTextureFactory::TTextureFactory(size_t w, size_t h, char const * resName)
    : BasePoolElemFactory(resName, w * h * sizeof(TDynamicTexture::pixel_t)),
      m_w(w), m_h(h)
  {}

  shared_ptr<gl::BaseTexture> const TTextureFactory::Create()
  {
    return shared_ptr<gl::BaseTexture>(new TDynamicTexture(m_w, m_h));
  }

  TStorageFactory::TStorageFactory(size_t vbSize, size_t ibSize, bool useVA, bool isMergeable, char const * resName)
    : BasePoolElemFactory(resName, vbSize + ibSize),
      m_vbSize(vbSize),
      m_ibSize(ibSize),
      m_useVA(useVA),
      m_isMergeable(isMergeable)
  {}

  gl::Storage const TStorageFactory::Create()
  {
    gl::Storage res(m_vbSize, m_ibSize, m_useVA);

    if (m_isMergeable)
    {
      res.m_indices->lock();
      res.m_vertices->lock();
    }

    return res;
  }

  void TStorageFactory::BeforeMerge(gl::Storage const & e)
  {
    if (m_isMergeable)
    {
      if (e.m_indices->isLocked())
        e.m_indices->unlock();

      e.m_indices->lock();

      if (e.m_vertices->isLocked())
        e.m_vertices->unlock();

      e.m_vertices->lock();
    }
  }

  ResourceManager::StoragePoolParams::StoragePoolParams()
    : m_vbSize(0), m_ibSize(0), m_storagesCount(0), m_isFixed(true)
  {}

  ResourceManager::StoragePoolParams::StoragePoolParams(size_t vbSize, size_t ibSize, size_t storagesCount, bool isFixed)
    : m_vbSize(vbSize), m_ibSize(ibSize), m_storagesCount(storagesCount), m_isFixed(isFixed)
  {}

  bool ResourceManager::StoragePoolParams::isValid() const
  {
    return m_vbSize && m_ibSize && m_storagesCount;
  }

  size_t ResourceManager::StoragePoolParams::memoryUsage() const
  {
    return (m_vbSize + m_ibSize) * m_storagesCount;
  }

  void ResourceManager::StoragePoolParams::scaleMemoryUsage(double k)
  {
    m_vbSize *= k;
    m_ibSize *= k;
  }

  ResourceManager::TexturePoolParams::TexturePoolParams()
    : m_texWidth(0), m_texHeight(0), m_texCount(0), m_rtFormat(yg::Rt8Bpp), m_isFixed(true)
  {}

  ResourceManager::TexturePoolParams::TexturePoolParams(size_t texWidth, size_t texHeight, size_t texCount, yg::RtFormat rtFormat, bool isFixed)
    : m_texWidth(texWidth), m_texHeight(texHeight), m_texCount(texCount), m_rtFormat(rtFormat), m_isFixed(isFixed)
  {}

  bool ResourceManager::TexturePoolParams::isValid() const
  {
    return m_texWidth && m_texHeight && m_texCount;
  }

  size_t ResourceManager::TexturePoolParams::memoryUsage() const
  {
    size_t pixelSize = 0;
    switch (m_rtFormat)
    {
    case yg::Rt4Bpp:
      pixelSize = 2;
      break;
    case yg::Rt8Bpp:
      pixelSize = 4;
      break;
    }

    return m_texWidth * m_texHeight * pixelSize * m_texCount;
  }

  void ResourceManager::TexturePoolParams::scaleMemoryUsage(double k)
  {
    if (k > 1)
    {
      while (k > 2)
      {
        if (k > 2)
        {
          m_texWidth *= 2;
          k /= 2;
        }

        if (k > 2)
        {
          m_texHeight *= 2;
          k /= 2;
        }
      }
    }
    else
    {
      if (k < 1)
      {
        while (k < 1)
        {
          if (k < 0.5)
          {
            m_texHeight /= 2;
            k *= 2;
          }

          if (k < 0.5)
          {
            m_texWidth /= 2;
            k *= 2;
          }
        }
      }
    }

    m_texCount *= k;
  }

  ResourceManager::GlyphCacheParams::GlyphCacheParams()
    : m_glyphCacheMemoryLimit(0),
      m_glyphCacheCount(0),
      m_renderThreadCount(0)
  {}

  ResourceManager::GlyphCacheParams::GlyphCacheParams(string const & unicodeBlockFile,
                                                      string const & whiteListFile,
                                                      string const & blackListFile,
                                                      size_t glyphCacheMemoryLimit,
                                                      size_t glyphCacheCount,
                                                      size_t renderThreadCount)
    : m_unicodeBlockFile(unicodeBlockFile),
      m_whiteListFile(whiteListFile),
      m_blackListFile(blackListFile),
      m_glyphCacheMemoryLimit(glyphCacheMemoryLimit),
      m_glyphCacheCount(glyphCacheCount),
      m_renderThreadCount(renderThreadCount)
  {}

  ResourceManager::Params::Params()
    : m_rtFormat(yg::Rt8Bpp),
      m_isMergeable(false),
      m_useVA(true),
      m_videoMemoryLimit(0)
  {}

  void ResourceManager::Params::fitIntoLimits()
  {
    size_t storageMemoryUsage = m_primaryStoragesParams.memoryUsage()
        + m_smallStoragesParams.memoryUsage()
        + m_blitStoragesParams.memoryUsage()
        + m_multiBlitStoragesParams.memoryUsage()
        + m_tinyStoragesParams.memoryUsage();

    size_t fixedStorageMemoryUsage = (m_primaryStoragesParams.m_isFixed ? m_primaryStoragesParams.memoryUsage() : 0)
                                   + (m_smallStoragesParams.m_isFixed ? m_smallStoragesParams.memoryUsage() : 0)
                                   + (m_blitStoragesParams.m_isFixed ? m_blitStoragesParams.memoryUsage() : 0)
                                   + (m_multiBlitStoragesParams.m_isFixed ? m_multiBlitStoragesParams.memoryUsage() : 0)
                                   + (m_tinyStoragesParams.m_isFixed ? m_tinyStoragesParams.memoryUsage() : 0);

    size_t textureMemoryUsage = m_primaryTexturesParams.memoryUsage()
        + m_fontTexturesParams.memoryUsage()
        + m_renderTargetTexturesParams.memoryUsage()
        + m_styleCacheTexturesParams.memoryUsage();

    size_t fixedTextureMemoryUsage = (m_primaryTexturesParams.m_isFixed ? m_primaryTexturesParams.memoryUsage() : 0)
                                   + (m_fontTexturesParams.m_isFixed ? m_fontTexturesParams.memoryUsage() : 0)
                                   + (m_renderTargetTexturesParams.m_isFixed ? m_renderTargetTexturesParams.memoryUsage() : 0)
                                   + (m_styleCacheTexturesParams.m_isFixed ? m_styleCacheTexturesParams.memoryUsage() : 0);

    size_t videoMemoryLimit = m_videoMemoryLimit;
    if (videoMemoryLimit == 0)
      videoMemoryLimit = storageMemoryUsage + textureMemoryUsage;

    double k = ((int)videoMemoryLimit - (int)fixedStorageMemoryUsage - (int)fixedTextureMemoryUsage)* 1.0 / ((int)storageMemoryUsage + (int)textureMemoryUsage - (int)fixedStorageMemoryUsage - (int)fixedTextureMemoryUsage);

    if (k < 0)
      k = 1;

    scaleMemoryUsage(k);

    storageMemoryUsage = m_primaryStoragesParams.memoryUsage()
        + m_smallStoragesParams.memoryUsage()
        + m_blitStoragesParams.memoryUsage()
        + m_multiBlitStoragesParams.memoryUsage()
        + m_tinyStoragesParams.memoryUsage();

    textureMemoryUsage = m_primaryTexturesParams.memoryUsage()
        + m_fontTexturesParams.memoryUsage()
        + m_renderTargetTexturesParams.memoryUsage()
        + m_styleCacheTexturesParams.memoryUsage();

    LOG(LINFO, ("allocating ", storageMemoryUsage + textureMemoryUsage, " bytes of videoMemory, with initial limit ", m_videoMemoryLimit));
  }

  void ResourceManager::Params::scaleMemoryUsage(double k)
  {
    if (!m_primaryStoragesParams.m_isFixed)
      m_primaryStoragesParams.scaleMemoryUsage(k);
    if (!m_smallStoragesParams.m_isFixed)
      m_smallStoragesParams.scaleMemoryUsage(k);
    if (!m_blitStoragesParams.m_isFixed)
      m_blitStoragesParams.scaleMemoryUsage(k);
    if (!m_multiBlitStoragesParams.m_isFixed)
      m_multiBlitStoragesParams.scaleMemoryUsage(k);
    if (!m_tinyStoragesParams.m_isFixed)
      m_tinyStoragesParams.scaleMemoryUsage(k);

    if (!m_primaryTexturesParams.m_isFixed)
      m_primaryTexturesParams.scaleMemoryUsage(k);
    if (!m_fontTexturesParams.m_isFixed)
      m_fontTexturesParams.scaleMemoryUsage(k);
    if (!m_renderTargetTexturesParams.m_isFixed)
      m_renderTargetTexturesParams.scaleMemoryUsage(k);
    if (!m_styleCacheTexturesParams.m_isFixed)
      m_styleCacheTexturesParams.scaleMemoryUsage(k);
  }

  ResourceManager::ResourceManager(Params const & p)
    : m_params(p)
  {
    initGlyphCaches(p.m_glyphCacheParams);

    initPrimaryStorage(p.m_primaryStoragesParams);
    initSmallStorage(p.m_smallStoragesParams);
    initBlitStorage(p.m_blitStoragesParams);
    initMultiBlitStorage(p.m_multiBlitStoragesParams);
    initTinyStorage(p.m_tinyStoragesParams);

    initPrimaryTextures(p.m_primaryTexturesParams);
    initFontTextures(p.m_fontTexturesParams);
    initRenderTargetTextures(p.m_renderTargetTexturesParams);
    initStyleCacheTextures(p.m_styleCacheTexturesParams);

    if (p.m_useVA)
      LOG(LINFO, ("buffer objects are unsupported. using client vertex array instead."));
  }

  void ResourceManager::initGlyphCaches(GlyphCacheParams const & p)
  {
    if (p.m_glyphCacheMemoryLimit && p.m_glyphCacheCount)
    {
      LOG(LDEBUG, ("allocating ", p.m_glyphCacheCount, " glyphCaches, ", p.m_glyphCacheMemoryLimit, " bytes total."));

      for (size_t i = 0; i < p.m_glyphCacheCount; ++i)
        m_glyphCaches.push_back(GlyphCache(GlyphCache::Params(p.m_unicodeBlockFile, p.m_whiteListFile, p.m_blackListFile, p.m_glyphCacheMemoryLimit / p.m_glyphCacheCount)));
    }
    else
      LOG(LERROR, ("no params to init glyph caches."));
  }

  void ResourceManager::initPrimaryStorage(StoragePoolParams const & p)
  {
    if (p.isValid())
    {
      if (m_params.m_isMergeable)
        m_primaryStorages.reset(new TMergeableStoragePoolImpl(new TMergeableStoragePoolTraits(TStorageFactory(p.m_vbSize, p.m_ibSize, m_params.m_useVA, m_params.m_isMergeable, "primaryStorage"), p.m_storagesCount)));
      else
        m_primaryStorages.reset(new TNonMergeableStoragePoolImpl(new TNonMergeableStoragePoolTraits(TStorageFactory(p.m_vbSize, p.m_ibSize, m_params.m_useVA, m_params.m_isMergeable, "primaryStorage"), p.m_storagesCount)));
    }
    else
      LOG(LINFO, ("no primary storages"));
  }

  void ResourceManager::initSmallStorage(StoragePoolParams const & p)
  {
    if (p.isValid())
    {
      if (m_params.m_isMergeable)
        m_smallStorages.reset(new TMergeableStoragePoolImpl(new TMergeableStoragePoolTraits(TStorageFactory(p.m_vbSize, p.m_ibSize, m_params.m_useVA, m_params.m_isMergeable, "smallStorage"), p.m_storagesCount)));
      else
        m_smallStorages.reset(new TNonMergeableStoragePoolImpl(new TNonMergeableStoragePoolTraits(TStorageFactory(p.m_vbSize, p.m_ibSize, m_params.m_useVA, m_params.m_isMergeable, "smallStorage"), p.m_storagesCount)));
    }
    else
      LOG(LINFO, ("no small storages"));
  }

  void ResourceManager::initBlitStorage(StoragePoolParams const & p)
  {
    if (p.isValid())
    {
      if (m_params.m_isMergeable)
        m_blitStorages.reset(new TMergeableStoragePoolImpl(new TMergeableStoragePoolTraits(TStorageFactory(p.m_vbSize, p.m_ibSize, m_params.m_useVA, m_params.m_isMergeable, "blitStorage"), p.m_storagesCount)));
      else
        m_blitStorages.reset(new TNonMergeableStoragePoolImpl(new TNonMergeableStoragePoolTraits(TStorageFactory(p.m_vbSize, p.m_ibSize, m_params.m_useVA, m_params.m_isMergeable, "blitStorage"), p.m_storagesCount)));
    }
    else
      LOG(LINFO, ("no blit storages"));
  }

  void ResourceManager::initMultiBlitStorage(StoragePoolParams const & p)
  {
    if (p.isValid())
    {
      if (m_params.m_isMergeable)
        m_multiBlitStorages.reset(new TMergeableStoragePoolImpl(new TMergeableStoragePoolTraits(TStorageFactory(p.m_vbSize, p.m_ibSize, m_params.m_useVA, m_params.m_isMergeable, "multiBlitStorage"), p.m_storagesCount)));
      else
        m_multiBlitStorages.reset(new TNonMergeableStoragePoolImpl(new TNonMergeableStoragePoolTraits(TStorageFactory(p.m_vbSize, p.m_ibSize, m_params.m_useVA, m_params.m_isMergeable, "multiBlitStorage"), p.m_storagesCount)));
    }
    else
      LOG(LINFO, ("no multiBlit storages"));
  }

  void ResourceManager::initTinyStorage(StoragePoolParams const & p)
  {
    if (p.isValid())
    {
      if (m_params.m_isMergeable)
        m_tinyStorages.reset(new TMergeableStoragePoolImpl(new TMergeableStoragePoolTraits(TStorageFactory(p.m_vbSize, p.m_ibSize, m_params.m_useVA, m_params.m_isMergeable, "tinyStorage"), p.m_storagesCount)));
      else
        m_tinyStorages.reset(new TNonMergeableStoragePoolImpl(new TNonMergeableStoragePoolTraits(TStorageFactory(p.m_vbSize, p.m_ibSize, m_params.m_useVA, m_params.m_isMergeable, "tinyStorage"), p.m_storagesCount)));
    }
    else
      LOG(LINFO, ("no tiny storages"));
  }

  void ResourceManager::initPrimaryTextures(TexturePoolParams const & p)
  {
    if (p.isValid())
      m_primaryTextures.reset(new TTexturePoolImpl(new TTexturePoolTraits(TTextureFactory(p.m_texWidth, p.m_texHeight, "primaryTextures"), p.m_texCount)));
    else
      LOG(LINFO, ("no primary textures"));
  }

  void ResourceManager::initFontTextures(TexturePoolParams const & p)
  {
    if (p.isValid())
      m_fontTextures.reset(new TTexturePoolImpl(new TTexturePoolTraits(TTextureFactory(p.m_texWidth, p.m_texHeight, "fontTextures"), p.m_texCount)));
    else
      LOG(LINFO, ("no font textures"));
  }

  void ResourceManager::initRenderTargetTextures(TexturePoolParams const & p)
  {
    if (p.isValid())
      m_renderTargets.reset(new TTexturePoolImpl(new TTexturePoolTraits(TTextureFactory(p.m_texWidth, p.m_texHeight, "renderTargets"), p.m_texCount)));
    else
      LOG(LINFO, ("no renderTarget textures"));
  }

  void ResourceManager::initStyleCacheTextures(TexturePoolParams const & p)
  {
    if (p.isValid())
      m_styleCacheTextures.reset(new TTexturePoolImpl(new TTexturePoolTraits(TTextureFactory(p.m_texWidth, p.m_texHeight, "styleCacheTextures"), p.m_texCount)));
    else
      LOG(LINFO, ("no styleCache textures"));
  }

  shared_ptr<gl::BaseTexture> const & ResourceManager::getTexture(string const & fileName)
  {
    TStaticTextures::const_iterator it = m_staticTextures.find(fileName);
    if (it != m_staticTextures.end())
      return it->second;

    shared_ptr<gl::BaseTexture> texture;

    texture = make_shared_ptr(new TStaticTexture(fileName));

    m_staticTextures[fileName] = texture;
    return m_staticTextures[fileName];
  }

  Skin * loadSkin(shared_ptr<ResourceManager> const & resourceManager, string const & fileName, size_t dynamicPagesCount, size_t textPagesCount)
  {
    if (fileName.empty())
      return 0;

    SkinLoader loader(resourceManager, dynamicPagesCount, textPagesCount);

    try
    {
      ReaderPtr<Reader> skinFile(GetPlatform().GetReader(fileName));
      ReaderSource<ReaderPtr<Reader> > source(skinFile);
      if (!ParseXML(source, loader))
        MYTHROW(RootException, ("Error parsing skin file: ", fileName));
    }
    catch (RootException const & e)
    {
      LOG(LERROR, ("Error reading skin file: ", e.what()));
      return 0;
    }

    return loader.skin();
  }

  ResourceManager::Params const & ResourceManager::params() const
  {
    return m_params;
  }

  GlyphCache * ResourceManager::glyphCache(int glyphCacheID)
  {
    return glyphCacheID == -1 ? 0 : &m_glyphCaches[glyphCacheID];
  }

  void ResourceManager::addFonts(vector<string> const & fontNames)
  {
    for (unsigned i = 0; i < m_glyphCaches.size(); ++i)
      m_glyphCaches[i].addFonts(fontNames);
  }

  void ResourceManager::memoryWarning()
  {
  }

  void ResourceManager::enterBackground()
  {
    threads::MutexGuard guard(m_mutex);

    if (m_primaryStorages.get())
      m_primaryStorages->EnterBackground();

    if (m_smallStorages.get())
      m_smallStorages->EnterBackground();

    if (m_blitStorages.get())
      m_blitStorages->EnterBackground();

    if (m_multiBlitStorages.get())
      m_multiBlitStorages->EnterBackground();

    if (m_tinyStorages.get())
      m_tinyStorages->EnterBackground();

    if (m_primaryTextures.get())
      m_primaryTextures->EnterBackground();

    if (m_fontTextures.get())
      m_fontTextures->EnterBackground();

    if (m_renderTargets.get())
      m_renderTargets->EnterBackground();

    if (m_styleCacheTextures.get())
      m_styleCacheTextures->EnterBackground();
  }

  void ResourceManager::enterForeground()
  {
    threads::MutexGuard guard(m_mutex);

    if (m_primaryStorages.get())
      m_primaryStorages->EnterForeground();

    if (m_smallStorages.get())
      m_smallStorages->EnterForeground();

    if (m_blitStorages.get())
      m_blitStorages->EnterForeground();

    if (m_multiBlitStorages.get())
      m_multiBlitStorages->EnterForeground();

    if (m_tinyStorages.get())
      m_tinyStorages->EnterForeground();

    if (m_primaryTextures.get())
      m_primaryTextures->EnterForeground();

    if (m_fontTextures.get())
      m_fontTextures->EnterForeground();

    if (m_renderTargets.get())
      m_renderTargets->EnterForeground();

    if (m_styleCacheTextures.get())
      m_styleCacheTextures->EnterForeground();
  }

  shared_ptr<yg::gl::BaseTexture> ResourceManager::createRenderTarget(unsigned w, unsigned h)
  {
    switch (m_params.m_rtFormat)
    {
    case Rt8Bpp:
      return make_shared_ptr(new gl::Texture<RGBA8Traits, false>(w, h));
    case Rt4Bpp:
      return make_shared_ptr(new gl::Texture<RGBA4Traits, false>(w, h));
    default:
      MYTHROW(ResourceManagerException, ("unknown render target format"));
    };
  }

  int ResourceManager::renderThreadGlyphCacheID(int threadNum) const
  {
    ASSERT(threadNum < m_params.m_glyphCacheParams.m_renderThreadCount, ());
    return 1 + threadNum;
  }

  int ResourceManager::guiThreadGlyphCacheID() const
  {
    return 0;
  }

  int ResourceManager::cacheThreadGlyphCacheID() const
  {
    return 1 + m_params.m_glyphCacheParams.m_renderThreadCount;
  }

  ResourceManager::TStoragePool * ResourceManager::primaryStorages()
  {
    return m_primaryStorages.get();
  }

  ResourceManager::TStoragePool * ResourceManager::smallStorages()
  {
    return m_smallStorages.get();
  }

  ResourceManager::TStoragePool * ResourceManager::tinyStorages()
  {
    return m_tinyStorages.get();
  }

  ResourceManager::TStoragePool * ResourceManager::blitStorages()
  {
    return m_blitStorages.get();
  }

  ResourceManager::TStoragePool * ResourceManager::multiBlitStorages()
  {
    return m_multiBlitStorages.get();
  }

  ResourceManager::TTexturePool * ResourceManager::primaryTextures()
  {
    return m_primaryTextures.get();
  }

  ResourceManager::TTexturePool * ResourceManager::fontTextures()
  {
    return m_fontTextures.get();
  }

  ResourceManager::TTexturePool * ResourceManager::renderTargetTextures()
  {
    return m_renderTargets.get();
  }

  ResourceManager::TTexturePool * ResourceManager::styleCacheTextures()
  {
    return m_styleCacheTextures.get();
  }

  void ResourceManager::mergeFreeResources()
  {
    if (m_tinyStorages.get())
      m_tinyStorages->Merge();
    if (m_primaryStorages.get())
      m_primaryStorages->Merge();
    if (m_smallStorages.get())
      m_smallStorages->Merge();
    if (m_blitStorages.get())
      m_blitStorages->Merge();
    if (m_multiBlitStorages.get())
      m_multiBlitStorages->Merge();
  }
}
