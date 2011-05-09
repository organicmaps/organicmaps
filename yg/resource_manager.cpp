#include "../base/SRC_FIRST.hpp"
#include "internal/opengl.hpp"
#include "base_texture.hpp"
#include "data_formats.hpp"
#include "resource_manager.hpp"
#include "skin_loader.hpp"
#include "texture.hpp"
#include "storage.hpp"

#include "../coding/file_reader.hpp"
#include "../coding/parse_xml.hpp"
#include "../base/logging.hpp"


namespace yg
{
  typedef gl::Texture<DATA_TRAITS, true> TDynamicTexture;
  typedef gl::Texture<DATA_TRAITS, false> TStaticTexture;

  ResourceManager::ResourceManager(size_t vbSize, size_t ibSize, size_t storagesCount,
                                   size_t smallVBSize, size_t smallIBSize, size_t smallStoragesCount,
                                   size_t blitVBSize, size_t blitIBSize, size_t blitStoragesCount,
                                   size_t texWidth, size_t texHeight, size_t texCount,
                                   char const * blocksFile, char const * whiteListFile, char const * blackListFile, size_t maxGlyphCacheSize,
                                   RtFormat fmt,
                                   bool useVA,
                                   bool fillSkinAlpha)
                                     : m_textureWidth(texWidth), m_textureHeight(texHeight),
                                     m_vbSize(vbSize), m_ibSize(ibSize),
                                     m_smallVBSize(smallVBSize), m_smallIBSize(smallIBSize),
                                     m_blitVBSize(blitVBSize), m_blitIBSize(blitIBSize),
                                     m_glyphCache(GlyphCache::Params(blocksFile, whiteListFile, blackListFile, maxGlyphCacheSize)),
                                     m_format(fmt),
                                     m_useVA(useVA),
                                     m_fillSkinAlpha(fillSkinAlpha)
  {
    if (useVA)
      LOG(LINFO, ("buffer objects are unsupported. using client vertex array instead."));

    for (size_t i = 0; i < storagesCount; ++i)
      m_storages.push_back(gl::Storage(vbSize, ibSize, m_useVA));

    LOG(LINFO, ("allocating ", (vbSize + ibSize) * storagesCount, " bytes for main storage"));

    for (size_t i = 0; i < smallStoragesCount; ++i)
      m_smallStorages.push_back(gl::Storage(smallVBSize, smallIBSize, m_useVA));

    LOG(LINFO, ("allocating ", (smallVBSize + smallIBSize) * smallStoragesCount, " bytes for small storage"));

    for (size_t i = 0; i < blitStoragesCount; ++i)
      m_blitStorages.push_back(gl::Storage(blitVBSize, blitIBSize, m_useVA));

    LOG(LINFO, ("allocating ", (blitVBSize + blitIBSize) * blitStoragesCount, " bytes for blit storage"));

    for (size_t i = 0; i < texCount; ++i)
    {
      m_dynamicTextures.push_back(shared_ptr<gl::BaseTexture>(new TDynamicTexture(texWidth, texHeight)));
#ifdef DEBUG
      static_cast<TDynamicTexture*>(m_dynamicTextures.back().get())->randomize();
#endif
    }

    LOG(LINFO, ("allocating ", texWidth * texHeight * sizeof(TDynamicTexture::pixel_t), " bytes for textures"));
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

  shared_ptr<gl::BaseTexture> const ResourceManager::reserveTexture(bool doWait)
  {
    threads::MutexGuard guard(m_mutex);

    if (m_dynamicTextures.empty())
    {
      if (doWait)
      {
        /// somehow wait
      }
    }

    shared_ptr<gl::BaseTexture> res = m_dynamicTextures.front();
    m_dynamicTextures.pop_front();
    return res;
  }

  size_t ResourceManager::textureWidth() const
  {
    return m_textureWidth;
  }

  size_t ResourceManager::textureHeight() const
  {
    return m_textureHeight;
  }

  void ResourceManager::freeTexture(shared_ptr<gl::BaseTexture> const & texture, bool doSignal)
  {
    threads::MutexGuard guard(m_mutex);

    bool needToSignal = m_dynamicTextures.empty();

    m_dynamicTextures.push_back(texture);

    if ((needToSignal) && (doSignal))
    {
      /// somehow signal
    }
  }

  shared_ptr<GlyphInfo> const ResourceManager::getGlyph(GlyphKey const & key)
  {
    return m_glyphCache.getGlyph(key);
  }

  GlyphMetrics const ResourceManager::getGlyphMetrics(GlyphKey const & key)
  {
    return m_glyphCache.getGlyphMetrics(key);
  }

  GlyphCache * ResourceManager::getGlyphCache()
  {
    return &m_glyphCache;
  }

  void ResourceManager::addFonts(vector<string> const & fontNames)
  {
    m_glyphCache.addFonts(fontNames);
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

    LOG(LINFO, ("freed ", m_storages.size(), " storages, ", m_smallStorages.size(), " small storages, ", m_blitStorages.size(), " blit storages and ", m_dynamicTextures.size(), " textures"));
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
      *it = shared_ptr<gl::BaseTexture>(new TDynamicTexture(m_textureWidth, m_textureHeight));
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
      throw std::runtime_error("unknows render target format");
    };
  }

  bool ResourceManager::fillSkinAlpha() const
  {
    return m_fillSkinAlpha;
  }
}
