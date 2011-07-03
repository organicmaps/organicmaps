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
                                   size_t tileTexWidth, size_t tileTexHeight, size_t tileTexCount,
                                   char const * blocksFile, char const * whiteListFile, char const * blackListFile,
                                   size_t glyphCacheSize,
                                   RtFormat fmt,
                                   bool useVA,
                                   bool fillSkinAlpha)
                                     : m_dynamicTextureWidth(dynamicTexWidth), m_dynamicTextureHeight(dynamicTexHeight),
                                       m_fontTextureWidth(fontTexWidth), m_fontTextureHeight(fontTexHeight),
                                       m_tileTextureWidth(tileTexWidth), m_tileTextureHeight(tileTexHeight),
                                     m_vbSize(vbSize), m_ibSize(ibSize),
                                     m_smallVBSize(smallVBSize), m_smallIBSize(smallIBSize),
                                     m_blitVBSize(blitVBSize), m_blitIBSize(blitIBSize),
                                     m_format(fmt),
                                     m_useVA(useVA),
                                     m_fillSkinAlpha(fillSkinAlpha)
  {
    /// primary cache is for rendering, so it's big
    m_glyphCaches.push_back(GlyphCache(GlyphCache::Params(blocksFile, whiteListFile, blackListFile, glyphCacheSize / 3)));
    m_glyphCaches.push_back(GlyphCache(GlyphCache::Params(blocksFile, whiteListFile, blackListFile, glyphCacheSize / 3)));
    m_glyphCaches.push_back(GlyphCache(GlyphCache::Params(blocksFile, whiteListFile, blackListFile, glyphCacheSize / 3)));

    if (useVA)
    {
      LOG(LINFO, ("buffer objects are unsupported. using client vertex array instead."));
    }

    for (size_t i = 0; i < storagesCount; ++i)
      m_storages.PushBack(gl::Storage(vbSize, ibSize, m_useVA));

    LOG(LINFO, ("allocating ", (vbSize + ibSize) * storagesCount, " bytes for main storage"));

    for (size_t i = 0; i < smallStoragesCount; ++i)
      m_smallStorages.PushBack(gl::Storage(smallVBSize, smallIBSize, m_useVA));

    LOG(LINFO, ("allocating ", (smallVBSize + smallIBSize) * smallStoragesCount, " bytes for small storage"));

    for (size_t i = 0; i < blitStoragesCount; ++i)
      m_blitStorages.PushBack(gl::Storage(blitVBSize, blitIBSize, m_useVA));

    LOG(LINFO, ("allocating ", (blitVBSize + blitIBSize) * blitStoragesCount, " bytes for blit storage"));

    for (size_t i = 0; i < dynamicTexCount; ++i)
    {
      shared_ptr<gl::BaseTexture> t(new TDynamicTexture(dynamicTexWidth, dynamicTexHeight));
      m_dynamicTextures.PushBack(t);
#ifdef DEBUG
      static_cast<TDynamicTexture*>(t.get())->randomize();
#endif
    }

    LOG(LINFO, ("allocating ", dynamicTexWidth * dynamicTexHeight * sizeof(TDynamicTexture::pixel_t), " bytes for textures"));

    for (size_t i = 0; i < fontTexCount; ++i)
    {
      shared_ptr<gl::BaseTexture> t(new TDynamicTexture(fontTexWidth, fontTexHeight));
      m_fontTextures.PushBack(t);
#ifdef DEBUG
      static_cast<TDynamicTexture*>(t.get())->randomize();
#endif
    }

    LOG(LINFO, ("allocating ", fontTexWidth * fontTexHeight * sizeof(TDynamicTexture::pixel_t), " bytes for font textures"));

    for (size_t i = 0; i < tileTexCount; ++i)
    {
      shared_ptr<gl::BaseTexture> t(new TStaticTexture(tileTexWidth, tileTexHeight));
      m_renderTargets.PushBack(t);
    }

    LOG(LINFO, ("allocating ", tileTexWidth * tileTexHeight * sizeof(TStaticTexture::pixel_t), " bytes for tiles"));
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
    return m_tileTextureWidth;
  }

  size_t ResourceManager::tileTextureHeight() const
  {
    return m_tileTextureHeight;
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

    m_storagesCount = m_storages.Size();
    m_smallStoragesCount = m_smallStorages.Size();
    m_blitStoragesCount = m_blitStorages.Size();
    m_dynamicTexturesCount = m_dynamicTextures.Size();
    m_fontTexturesCount = m_fontTextures.Size();

    m_storages.Clear();
    m_smallStorages.Clear();
    m_blitStorages.Clear();
    m_dynamicTextures.Clear();
    m_fontTextures.Clear();

    LOG(LINFO, ("freed ", m_storagesCount, " storages, ", m_smallStoragesCount, " small storages, ", m_blitStoragesCount, " blit storages, ", m_dynamicTexturesCount, " dynamic textures and ", m_fontTexturesCount, " font textures"));
  }

  void ResourceManager::enterForeground()
  {
    threads::MutexGuard guard(m_mutex);

    for (size_t i = 0; i < m_storagesCount; ++i)
      m_storages.PushBack(gl::Storage(m_vbSize, m_ibSize, m_useVA));
    for (size_t i = 0; i < m_smallStoragesCount; ++i)
      m_smallStorages.PushBack(gl::Storage(m_smallVBSize, m_smallIBSize, m_useVA));
    for (size_t i = 0; i < m_blitStoragesCount; ++i)
      m_blitStorages.PushBack(gl::Storage(m_blitVBSize, m_blitIBSize, m_useVA));

    for (size_t i = 0; i < m_dynamicTexturesCount; ++i)
      m_dynamicTextures.PushBack(shared_ptr<gl::BaseTexture>(new TDynamicTexture(m_dynamicTextureWidth, m_dynamicTextureHeight)));
    for (size_t i = 0; i < m_fontTexturesCount; ++i)
      m_fontTextures.PushBack(shared_ptr<gl::BaseTexture>(new TDynamicTexture(m_fontTextureWidth, m_fontTextureHeight)));
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

  int ResourceManager::renderThreadGlyphCacheID(int threadNum) const
  {
    return 1 + threadNum;
  }

  int ResourceManager::guiThreadGlyphCacheID() const
  {
    return 0;
  }

  ThreadedList<gl::Storage> & ResourceManager::storages()
  {
    return m_storages;
  }

  ThreadedList<gl::Storage> & ResourceManager::smallStorages()
  {
    return m_smallStorages;
  }

  ThreadedList<gl::Storage> & ResourceManager::blitStorages()
  {
    return m_blitStorages;
  }

  ThreadedList<shared_ptr<gl::BaseTexture> > & ResourceManager::dynamicTextures()
  {
    return m_dynamicTextures;
  }

  ThreadedList<shared_ptr<gl::BaseTexture> > & ResourceManager::fontTextures()
  {
    return m_fontTextures;
  }

  ThreadedList<shared_ptr<gl::BaseTexture> > & ResourceManager::renderTargets()
  {
    return m_renderTargets;
  }
}
