#pragma once

#include "../std/shared_ptr.hpp"
#include "../std/map.hpp"
#include "../std/string.hpp"
#include "../std/list.hpp"
#include "../std/auto_ptr.hpp"

#include "../base/mutex.hpp"
#include "../base/resource_pool.hpp"

#include "storage.hpp"
#include "glyph_cache.hpp"

namespace yg
{
  class Skin;

  namespace gl
  {
    class BaseTexture;
    class Storage;
  }

  struct GlyphInfo;

  enum RtFormat
  {
    Rt8Bpp,
    Rt4Bpp
  };

  struct TTextureFactory : BasePoolElemFactory
  {
    size_t m_w;
    size_t m_h;
    TTextureFactory(size_t w, size_t h, char const * resName);
    shared_ptr<gl::BaseTexture> const Create();
  };

  struct TStorageFactory : BasePoolElemFactory
  {
    size_t m_vbSize;
    size_t m_ibSize;
    bool m_useVA;
    TStorageFactory(size_t vbSize, size_t ibSize, bool useVA, char const * resName);
    gl::Storage const Create();
    void BeforeMerge(gl::Storage const & e);
  };

  class ResourceManager
  {
  public:

    typedef BasePoolTraits<shared_ptr<gl::BaseTexture>, TTextureFactory> TBaseTexturePoolTraits;
    typedef FixedSizePoolTraits<TTextureFactory, TBaseTexturePoolTraits > TTexturePoolTraits;
    typedef ResourcePool<TTexturePoolTraits> TTexturePool;

    typedef BasePoolTraits<gl::Storage, TStorageFactory> TBaseStoragePoolTraits;
    typedef SeparateFreePoolTraits<TStorageFactory, TBaseStoragePoolTraits> TSeparateFreeStoragePoolTraits;
    typedef FixedSizePoolTraits<TStorageFactory, TSeparateFreeStoragePoolTraits> TStoragePoolTraits;
    typedef ResourcePool<TStoragePoolTraits> TStoragePool;

  private:

    typedef map<string, shared_ptr<gl::BaseTexture> > TStaticTextures;

    TStaticTextures m_staticTextures;

    threads::Mutex m_mutex;

    size_t m_dynamicTextureWidth;
    size_t m_dynamicTextureHeight;

    auto_ptr<TTexturePool> m_dynamicTextures;

    size_t m_fontTextureWidth;
    size_t m_fontTextureHeight;

    auto_ptr<TTexturePool> m_fontTextures;

    size_t m_styleCacheTextureWidth;
    size_t m_styleCacheTextureHeight;

    auto_ptr<TTexturePool> m_styleCacheTextures;

    size_t m_renderTargetWidth;
    size_t m_renderTargetHeight;

    auto_ptr<TTexturePool> m_renderTargets;

    size_t m_vbSize;
    size_t m_ibSize;

    size_t m_smallVBSize;
    size_t m_smallIBSize;

    size_t m_blitVBSize;
    size_t m_blitIBSize;

    size_t m_multiBlitVBSize;
    size_t m_multiBlitIBSize;

    size_t m_tinyVBSize;
    size_t m_tinyIBSize;

    auto_ptr<TStoragePool> m_storages;
    auto_ptr<TStoragePool> m_smallStorages;
    auto_ptr<TStoragePool> m_blitStorages;
    auto_ptr<TStoragePool> m_multiBlitStorages;
    auto_ptr<TStoragePool> m_tinyStorages;

    vector<GlyphCache> m_glyphCaches;

    RtFormat m_format;

    bool m_useVA;

  public:

    ResourceManager(size_t vbSize, size_t ibSize, size_t storagesCount,
                    size_t smallVBSize, size_t smallIBSize, size_t smallStoragesCount,
                    size_t blitVBSize, size_t blitIBSize, size_t blitStoragesCount,
                    size_t texWidth, size_t texHeight, size_t texCount,
                    size_t fontTexWidth, size_t fontTexHeight, size_t fontTexCount,
                    char const * blocksFile, char const * whileListFile, char const * blackListFile,
                    size_t glyphCacheSize,
                    size_t glyphCacheCount,
                    RtFormat fmt,
                    bool useVA);

    void initMultiBlitStorage(size_t multiBlitVBSize, size_t multiBlitIBSize, size_t multiBlitStoragesCount);
    void initRenderTargets(size_t renderTargetWidth, size_t renderTargetHeight, size_t renderTargetCount);
    void initTinyStorage(size_t tinyVBSize, size_t tinyIBSize, size_t tinyStoragesCount);
    void initStyleCacheTextures(size_t styleCacheTextureWidth, size_t styleCacheTextureHeight, size_t styleCacheTexturesCount);

    shared_ptr<gl::BaseTexture> const & getTexture(string const & fileName);

    TStoragePool * storages();
    TStoragePool * smallStorages();
    TStoragePool * blitStorages();
    TStoragePool * multiBlitStorages();
    TStoragePool * tinyStorages();

    TTexturePool * dynamicTextures();
    TTexturePool * fontTextures();
    TTexturePool * renderTargets();
    TTexturePool * styleCacheTextures();

    size_t dynamicTextureWidth() const;
    size_t dynamicTextureHeight() const;

    size_t fontTextureWidth() const;
    size_t fontTextureHeight() const;

    size_t tileTextureWidth() const;
    size_t tileTextureHeight() const;

    shared_ptr<GlyphInfo> const getGlyphInfo(GlyphKey const & key);
    GlyphMetrics const getGlyphMetrics(GlyphKey const & key);
    GlyphCache * glyphCache(int glyphCacheID = 0);

    int renderThreadGlyphCacheID(int threadNum) const;
    int guiThreadGlyphCacheID() const;
    int cacheThreadGlyphCacheID() const;

    void addFonts(vector<string> const & fontNames);

    void memoryWarning();
    void enterBackground();
    void enterForeground();

    void mergeFreeResources();

    shared_ptr<yg::gl::BaseTexture> createRenderTarget(unsigned w, unsigned h);
  };

  Skin * loadSkin(shared_ptr<ResourceManager> const & resourceManager,
                  string const & fileName,
                  size_t dynamicPagesCount,
                  size_t textPagesCount);
}

