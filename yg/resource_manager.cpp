#include "internal/opengl.hpp"
#include "base_texture.hpp"
#include "data_formats.hpp"
#include "resource_manager.hpp"
#include "skin_loader.hpp"
#include "storage.hpp"
#include "texture.hpp"

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

  TStorageFactory::TStorageFactory(size_t vbSize, size_t ibSize, bool useVA, char const * resName)
    : BasePoolElemFactory(resName, vbSize + ibSize),
      m_vbSize(vbSize), m_ibSize(ibSize), m_useVA(useVA)
  {}

  gl::Storage const TStorageFactory::Create()
  {
    return gl::Storage(m_vbSize, m_ibSize, m_useVA);
  }

  ResourceManager::ResourceManager(size_t vbSize, size_t ibSize, size_t storagesCount,
                                   size_t smallVBSize, size_t smallIBSize, size_t smallStoragesCount,
                                   size_t blitVBSize, size_t blitIBSize, size_t blitStoragesCount,
                                   size_t dynamicTexWidth, size_t dynamicTexHeight, size_t dynamicTexCount,
                                   size_t fontTexWidth, size_t fontTexHeight, size_t fontTexCount,
                                   char const * blocksFile, char const * whiteListFile, char const * blackListFile,
                                   size_t glyphCacheSize,
                                   size_t glyphCacheCount,
                                   RtFormat fmt,
                                   bool useVA)
                                     : m_dynamicTextureWidth(dynamicTexWidth), m_dynamicTextureHeight(dynamicTexHeight),
                                       m_fontTextureWidth(fontTexWidth), m_fontTextureHeight(fontTexHeight),
                                     m_vbSize(vbSize), m_ibSize(ibSize),
                                     m_smallVBSize(smallVBSize), m_smallIBSize(smallIBSize),
                                     m_blitVBSize(blitVBSize), m_blitIBSize(blitIBSize),
                                     m_format(fmt),
                                     m_useVA(useVA)
  {
    LOG(LDEBUG, ("allocating ", glyphCacheCount, " glyphCaches, ", glyphCacheSize, " bytes total."));

    for (size_t i = 0; i < glyphCacheCount; ++i)
      m_glyphCaches.push_back(GlyphCache(GlyphCache::Params(blocksFile, whiteListFile, blackListFile, glyphCacheSize / glyphCacheCount)));

    if (useVA)
      LOG(LINFO, ("buffer objects are unsupported. using client vertex array instead."));

    m_storages.reset(new TStoragePool(TStoragePoolTraits(TStorageFactory(vbSize, ibSize, useVA, "primaryStorage"), storagesCount)));
    m_smallStorages.reset(new TStoragePool(TStoragePoolTraits(TStorageFactory(smallVBSize, smallIBSize, useVA, "smallStorage"), smallStoragesCount)));
    m_blitStorages.reset(new TStoragePool(TStoragePoolTraits(TStorageFactory(blitVBSize, blitIBSize, useVA, "blitStorage"), blitStoragesCount)));

    m_dynamicTextures.reset(new TTexturePool(TTexturePoolTraits(TTextureFactory(dynamicTexWidth, dynamicTexHeight, "dynamicTexture"), dynamicTexCount)));
    m_fontTextures.reset(new TTexturePool(TTexturePoolTraits(TTextureFactory(fontTexWidth, fontTexHeight, "fontTexture"), fontTexCount)));
  }

  void ResourceManager::initMultiBlitStorage(size_t multiBlitVBSize, size_t multiBlitIBSize, size_t multiBlitStoragesCount)
  {
    m_multiBlitVBSize = multiBlitVBSize;
    m_multiBlitIBSize = multiBlitIBSize;

    m_multiBlitStorages.reset(new TStoragePool(TStoragePoolTraits(TStorageFactory(multiBlitVBSize, multiBlitIBSize, m_useVA, "multiBlitStorage"), multiBlitStoragesCount)));
  }

  void ResourceManager::initTinyStorage(size_t tinyVBSize, size_t tinyIBSize, size_t tinyStoragesCount)
  {
    m_tinyVBSize = tinyVBSize;
    m_tinyIBSize = tinyIBSize;

    m_tinyStorages.reset(new TStoragePool(TStoragePoolTraits(TStorageFactory(tinyVBSize, tinyIBSize, m_useVA, "tinyStorage"), tinyStoragesCount)));
  }

  void ResourceManager::initRenderTargets(size_t renderTargetWidth, size_t renderTargetHeight, size_t renderTargetsCount)
  {
    m_renderTargetWidth = renderTargetWidth;
    m_renderTargetHeight = renderTargetHeight;

    m_renderTargets.reset(new TTexturePool(TTexturePoolTraits(TTextureFactory(renderTargetWidth, renderTargetHeight, "renderTargets"), renderTargetsCount)));
  }

  void ResourceManager::initStyleCacheTextures(size_t styleCacheTextureWidth, size_t styleCacheTextureHeight, size_t styleCacheTexturesCount)
  {
    m_styleCacheTextureWidth = styleCacheTextureWidth;
    m_styleCacheTextureHeight = styleCacheTextureHeight;

    m_styleCacheTextures.reset(new TTexturePool(TTexturePoolTraits(TTextureFactory(styleCacheTextureWidth, styleCacheTextureHeight, "styleCacheTextures"), styleCacheTexturesCount)));
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

  size_t ResourceManager::dynamicTextureWidth() const
  {
    return m_dynamicTextureWidth;
  }

  size_t ResourceManager::dynamicTextureHeight() const
  {
    return m_dynamicTextureHeight;
  }

  size_t ResourceManager::fontTextureWidth() const
  {
    return m_fontTextureWidth;
  }

  size_t ResourceManager::fontTextureHeight() const
  {
    return m_fontTextureHeight;
  }

  size_t ResourceManager::tileTextureWidth() const
  {
    return m_renderTargetWidth;
  }

  size_t ResourceManager::tileTextureHeight() const
  {
    return m_renderTargetHeight;
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

    if (m_storages.get())
    m_storages->EnterBackground();

    if (m_smallStorages.get())
      m_smallStorages->EnterBackground();

    if (m_blitStorages.get())
      m_blitStorages->EnterBackground();

    if (m_multiBlitStorages.get())
      m_multiBlitStorages->EnterBackground();

    if (m_tinyStorages.get())
      m_tinyStorages->EnterBackground();

    if (m_dynamicTextures.get())
      m_dynamicTextures->EnterBackground();

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

    if (m_storages.get())
      m_storages->EnterForeground();

    if (m_smallStorages.get())
      m_smallStorages->EnterForeground();

    if (m_blitStorages.get())
      m_blitStorages->EnterForeground();

    if (m_multiBlitStorages.get())
      m_multiBlitStorages->EnterForeground();

    if (m_tinyStorages.get())
      m_tinyStorages->EnterForeground();

    if (m_dynamicTextures.get())
      m_dynamicTextures->EnterForeground();

    if (m_fontTextures.get())
      m_fontTextures->EnterForeground();

    if (m_renderTargets.get())
      m_renderTargets->EnterForeground();

    if (m_styleCacheTextures.get())
      m_styleCacheTextures->EnterForeground();
  }

  shared_ptr<yg::gl::BaseTexture> ResourceManager::createRenderTarget(unsigned w, unsigned h)
  {
    switch (m_format)
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
    return 2 + threadNum;
  }

  int ResourceManager::guiThreadGlyphCacheID() const
  {
    return 0;
  }

  int ResourceManager::cacheThreadGlyphCacheID() const
  {
    return 1;
  }

  ResourceManager::TStoragePool * ResourceManager::storages()
  {
    return m_storages.get();
  }

  ResourceManager::TStoragePool * ResourceManager::smallStorages()
  {
    return m_smallStorages.get();
  }

  ResourceManager::TStoragePool * ResourceManager::blitStorages()
  {
    return m_blitStorages.get();
  }

  ResourceManager::TStoragePool * ResourceManager::multiBlitStorages()
  {
    return m_multiBlitStorages.get();
  }

  ResourceManager::TStoragePool * ResourceManager::tinyStorages()
  {
    return m_tinyStorages.get();
  }

  ResourceManager::TTexturePool * ResourceManager::dynamicTextures()
  {
    return m_dynamicTextures.get();
  }

  ResourceManager::TTexturePool * ResourceManager::fontTextures()
  {
    return m_fontTextures.get();
  }

  ResourceManager::TTexturePool * ResourceManager::renderTargets()
  {
    return m_renderTargets.get();
  }

  ResourceManager::TTexturePool * ResourceManager::styleCacheTextures()
  {
    return m_styleCacheTextures.get();
  }
}
