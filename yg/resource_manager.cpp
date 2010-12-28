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
#include "../base/ptr_utils.hpp"

namespace yg
{
  typedef gl::Texture<DATA_TRAITS, true> TDynamicTexture;
  typedef gl::Texture<DATA_TRAITS, false> TStaticTexture;

  ResourceManager::ResourceManager(size_t vbSize, size_t ibSize, size_t storagesCount,
                                   size_t smallVBSize, size_t smallIBSize, size_t smallStoragesCount,
                                   size_t blitVBSize, size_t blitIBSize, size_t blitStoragesCount,
                                   size_t texWidth, size_t texHeight, size_t texCount,
                                   size_t maxGlyphCacheSize) : m_glyphCache(maxGlyphCacheSize)
  {
    for (size_t i = 0; i < storagesCount; ++i)
      m_storages.push_back(gl::Storage(vbSize, ibSize));

    for (size_t i = 0; i < smallStoragesCount; ++i)
      m_smallStorages.push_back(gl::Storage(smallVBSize, smallIBSize));

    for (size_t i = 0; i < blitStoragesCount; ++i)
      m_blitStorages.push_back(gl::Storage(blitVBSize, blitIBSize));

    for (size_t i = 0; i < texCount; ++i)
    {
      m_dynamicTextures.push_back(shared_ptr<gl::BaseTexture>(new TDynamicTexture(texWidth, texHeight)));
#ifdef DEBUG
      static_cast<TDynamicTexture*>(m_dynamicTextures.back().get())->randomize();
#endif
    }
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

  Skin * loadSkin(shared_ptr<ResourceManager> const & resourceManager, string const & fileName)
  {
    if (fileName.empty())
      return 0;
    SkinLoader loader(resourceManager);
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

  void ResourceManager::addFont(char const * fileName)
  {
    m_glyphCache.addFont(fileName);
  }
}
