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

  ResourceManager::ResourceManager(size_t vbSize, size_t ibSize, size_t storagesCount,
                                   size_t smallVBSize, size_t smallIBSize, size_t smallStoragesCount,
                                   size_t blitVBSize, size_t blitIBSize, size_t blitStoragesCount,
                                   size_t dynamicTexWidth, size_t dynamicTexHeight, size_t dynamicTexCount,
                                   size_t fontTexWidth, size_t fontTexHeight, size_t fontTexCount,
                                   char const * blocksFile, char const * whiteListFile, char const * blackListFile,
                                   size_t primaryGlyphCacheSize,
                                   size_t secondaryGlyphCacheSize,
                                   RtFormat fmt,
                                   bool useVA,
                                   bool fillSkinAlpha)
                                     : m_dynamicTextureWidth(dynamicTexWidth), m_dynamicTextureHeight(dynamicTexHeight),
                                       m_fontTextureWidth(fontTexWidth), m_fontTextureHeight(fontTexHeight),
                                     m_vbSize(vbSize), m_ibSize(ibSize),
                                     m_smallVBSize(smallVBSize), m_smallIBSize(smallIBSize),
                                     m_blitVBSize(blitVBSize), m_blitIBSize(blitIBSize),
                                     m_format(fmt),
                                     m_useVA(useVA),
                                     m_fillSkinAlpha(fillSkinAlpha)
  {
    /// primary cache is for rendering, so it's big
    m_glyphCaches.push_back(GlyphCache(GlyphCache::Params(blocksFile, whiteListFile, blackListFile, primaryGlyphCacheSize)));
    /// secondary caches is for glyph metrics only, so they are small
    m_glyphCaches.push_back(GlyphCache(GlyphCache::Params(blocksFile, whiteListFile, blackListFile, secondaryGlyphCacheSize)));

    if (useVA)
    {
      LOG(LINFO, ("buffer objects are unsupported. using client vertex array instead."));
    }

    for (size_t i = 0; i < storagesCount; ++i)
      m_storages.push_back(gl::Storage(vbSize, ibSize, m_useVA));

    LOG(LINFO, ("allocating ", (vbSize + ibSize) * storagesCount, " bytes for main storage"));

    for (size_t i = 0; i < smallStoragesCount; ++i)
      m_smallStorages.push_back(gl::Storage(smallVBSize, smallIBSize, m_useVA));

    LOG(LINFO, ("allocating ", (smallVBSize + smallIBSize) * smallStoragesCount, " bytes for small storage"));

    for (size_t i = 0; i < blitStoragesCount; ++i)
      m_blitStorages.push_back(gl::Storage(blitVBSize, blitIBSize, m_useVA));

    LOG(LINFO, ("allocating ", (blitVBSize + blitIBSize) * blitStoragesCount, " bytes for blit storage"));

    for (size_t i = 0; i < dynamicTexCount; ++i)
    {
      m_dynamicTextures.push_back(shared_ptr<gl::BaseTexture>(new TDynamicTexture(dynamicTexWidth, dynamicTexHeight)));
#ifdef DEBUG
      static_cast<TDynamicTexture*>(m_dynamicTextures.back().get())->randomize();
#endif
    }

    LOG(LINFO, ("allocating ", dynamicTexWidth * dynamicTexHeight * sizeof(TDynamicTexture::pixel_t), " bytes for textures"));

    for (size_t i = 0; i < fontTexCount; ++i)
    {
      m_fontTextures.push_back(shared_ptr<gl::BaseTexture>(new TDynamicTexture(fontTexWidth, fontTexHeight)));
#ifdef DEBUG
      static_cast<TDynamicTexture*>(m_fontTextures.back().get())->randomize();
#endif
    }

    LOG(LINFO, ("allocating ", fontTexWidth * fontTexHeight * sizeof(TDynamicTexture::pixel_t), " bytes for font textures"));
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
    FileReader skinFile(GetPlatform().ReadPathForFile(fileName));
    ReaderSource<FileReader> source(skinFile);
    bool parseResult = ParseXML(source, loader);
    ASSERT(parseResult, ("Invalid skin file structure?"));
    if (!parseResult)
      throw std::exception();
    return loader.skin();
  }

  gl::Storage const ResourceManager::reserveStorageImpl(bool doWait, list<gl::Storage> & l)
  {
    threads::MutexGuard guard(m_mutex);

    if (l.empty())
    {
      if (doWait)
      {
        /// somehow wait
      }
    }

    gl::Storage res = l.front();
    l.pop_front();
    return res;
  }

  void ResourceManager::freeStorageImpl(gl::Storage const & storage, bool doSignal, list<gl::Storage> & l)
  {
    threads::MutexGuard guard(m_mutex);

    bool needToSignal = l.empty();

    l.push_back(storage);

    if ((needToSignal) && (doSignal))
    {
      /// somehow signal
    }
  }

  gl::Storage const ResourceManager::reserveSmallStorage(bool doWait)
  {
    return reserveStorageImpl(doWait, m_smallStorages);
  }

  void ResourceManager::freeSmallStorage(gl::Storage const & storage, bool doSignal)
  {
    return freeStorageImpl(storage, doSignal, m_smallStorages);
  }

  gl::Storage const ResourceManager::reserveBlitStorage(bool doWait)
  {
    return reserveStorageImpl(doWait, m_blitStorages);
  }

  void ResourceManager::freeBlitStorage(gl::Storage const & storage, bool doSignal)
  {
    return freeStorageImpl(storage, doSignal, m_blitStorages);
  }

  gl::Storage const ResourceManager::reserveStorage(bool doWait)
  {
    return reserveStorageImpl(doWait, m_storages);
  }

  void ResourceManager::freeStorage(gl::Storage const & storage, bool doSignal)
  {
    return freeStorageImpl(storage, doSignal, m_storages);
  }

  shared_ptr<gl::BaseTexture> const ResourceManager::reserveTextureImpl(bool doWait, list<shared_ptr<gl::BaseTexture> > & l)
  {
    threads::MutexGuard guard(m_mutex);

    if (l.empty())
    {
      if (doWait)
      {
        /// somehow wait
      }
    }

    shared_ptr<gl::BaseTexture> res = l.front();
    l.pop_front();
    return res;
  }

  void ResourceManager::freeTextureImpl(shared_ptr<gl::BaseTexture> const & texture, bool doSignal, list<shared_ptr<gl::BaseTexture> > & l)
  {
    threads::MutexGuard guard(m_mutex);

    bool needToSignal = l.empty();

    l.push_back(texture);

    if ((needToSignal) && (doSignal))
    {
      /// somehow signal
    }
  }


  shared_ptr<gl::BaseTexture> const ResourceManager::reserveDynamicTexture(bool doWait)
  {
    return reserveTextureImpl(doWait, m_dynamicTextures);
  }

  void ResourceManager::freeDynamicTexture(shared_ptr<gl::BaseTexture> const & texture, bool doSignal)
  {
    freeTextureImpl(texture, doSignal, m_dynamicTextures);
  }


  size_t ResourceManager::dynamicTextureWidth() const
  {
    return m_dynamicTextureWidth;
  }

  size_t ResourceManager::dynamicTextureHeight() const
  {
    return m_dynamicTextureHeight;
  }

  shared_ptr<gl::BaseTexture> const ResourceManager::reserveFontTexture(bool doWait)
  {
    return reserveTextureImpl(doWait, m_fontTextures);
  }

  void ResourceManager::freeFontTexture(shared_ptr<gl::BaseTexture> const & texture, bool doSignal)
  {
    freeTextureImpl(texture, doSignal, m_fontTextures);
  }

  size_t ResourceManager::fontTextureWidth() const
  {
    return m_fontTextureWidth;
  }

  size_t ResourceManager::fontTextureHeight() const
  {
    return m_fontTextureHeight;
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

    for (list<gl::Storage>::iterator it = m_storages.begin(); it != m_storages.end(); ++it)
      *it = gl::Storage();
    for (list<gl::Storage>::iterator it = m_smallStorages.begin(); it != m_smallStorages.end(); ++it)
      *it = gl::Storage();
    for (list<gl::Storage>::iterator it = m_blitStorages.begin(); it != m_blitStorages.end(); ++it)
      *it = gl::Storage();

    for (list<shared_ptr<gl::BaseTexture> >::iterator it = m_dynamicTextures.begin(); it != m_dynamicTextures.end(); ++it)
      it->reset();

    for (list<shared_ptr<gl::BaseTexture> >::iterator it = m_fontTextures.begin(); it != m_fontTextures.end(); ++it)
      it->reset();

    LOG(LINFO, ("freed ", m_storages.size(), " storages, ", m_smallStorages.size(), " small storages, ", m_blitStorages.size(), " blit storages, ", m_dynamicTextures.size(), " dynamic textures and ", m_fontTextures.size(), " font textures"));
  }

  void ResourceManager::enterForeground()
  {
    threads::MutexGuard guard(m_mutex);

    for (list<gl::Storage>::iterator it = m_storages.begin(); it != m_storages.end(); ++it)
      *it = gl::Storage(m_vbSize, m_ibSize, m_useVA);
    for (list<gl::Storage>::iterator it = m_smallStorages.begin(); it != m_smallStorages.end(); ++it)
      *it = gl::Storage(m_smallVBSize, m_smallIBSize, m_useVA);
    for (list<gl::Storage>::iterator it = m_blitStorages.begin(); it != m_blitStorages.end(); ++it)
      *it = gl::Storage(m_blitVBSize, m_blitIBSize, m_useVA);

    for (list<shared_ptr<gl::BaseTexture> >::iterator it = m_dynamicTextures.begin(); it != m_dynamicTextures.end(); ++it)
      *it = shared_ptr<gl::BaseTexture>(new TDynamicTexture(m_dynamicTextureWidth, m_dynamicTextureHeight));
    for (list<shared_ptr<gl::BaseTexture> >::iterator it = m_fontTextures.begin(); it != m_fontTextures.end(); ++it)
      *it = shared_ptr<gl::BaseTexture>(new TDynamicTexture(m_fontTextureWidth, m_fontTextureHeight));
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

  bool ResourceManager::fillSkinAlpha() const
  {
    return m_fillSkinAlpha;
  }

  int ResourceManager::renderThreadGlyphCacheID() const
  {
    return 0;
  }

  int ResourceManager::guiThreadGlyphCacheID() const
  {
    return 1;
  }
}
