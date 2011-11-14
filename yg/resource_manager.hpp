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
    bool m_isMergeable;
    TStorageFactory(size_t vbSize, size_t ibSize, bool useVA, bool isMergeable, char const * resName);
    gl::Storage const Create();
    void BeforeMerge(gl::Storage const & e);
  };

  class ResourceManager
  {
  public:

    typedef BasePoolTraits<shared_ptr<gl::BaseTexture>, TTextureFactory> TBaseTexturePoolTraits;
    typedef FixedSizePoolTraits<TTextureFactory, TBaseTexturePoolTraits > TTexturePoolTraits;
    typedef ResourcePoolImpl<TTexturePoolTraits> TTexturePoolImpl;

    typedef ResourcePool<shared_ptr<gl::BaseTexture> > TTexturePool;

    typedef BasePoolTraits<gl::Storage, TStorageFactory> TBaseStoragePoolTraits;
    typedef SeparateFreePoolTraits<TStorageFactory, TBaseStoragePoolTraits> TSeparateFreeStoragePoolTraits;

    typedef FixedSizePoolTraits<TStorageFactory, TSeparateFreeStoragePoolTraits> TMergeableStoragePoolTraits;
    typedef ResourcePoolImpl<TMergeableStoragePoolTraits> TMergeableStoragePoolImpl;

    typedef FixedSizePoolTraits<TStorageFactory, TBaseStoragePoolTraits> TNonMergeableStoragePoolTraits;
    typedef ResourcePoolImpl<TNonMergeableStoragePoolTraits> TNonMergeableStoragePoolImpl;

    typedef ResourcePool<gl::Storage> TStoragePool;

  public:

    struct StoragePoolParams
    {
      size_t m_vbSize;
      size_t m_ibSize;
      size_t m_storagesCount;
      bool m_isFixed; //< should this params be scaled while fitting into videoMemoryLimit

      StoragePoolParams(size_t vbSize, size_t ibSize, size_t storagesCount, bool isFixed);
      StoragePoolParams();

      bool isValid() const;
      void scaleMemoryUsage(double k);
      size_t memoryUsage() const;
    };

    struct TexturePoolParams
    {
      size_t m_texWidth;
      size_t m_texHeight;
      size_t m_texCount;
      yg::RtFormat m_rtFormat;
      bool m_isFixed; //< should this params be scaled while fitting into videoMemoryLimit

      TexturePoolParams(size_t texWidth, size_t texHeight, size_t texCount, yg::RtFormat rtFormat, bool isFixed);
      TexturePoolParams();

      bool isValid() const;
      void scaleMemoryUsage(double k);
      size_t memoryUsage() const;
    };

    struct GlyphCacheParams
    {
      string m_unicodeBlockFile;
      string m_whiteListFile;
      string m_blackListFile;

      size_t m_glyphCacheMemoryLimit;
      size_t m_glyphCacheCount;
      size_t m_renderThreadCount;

      GlyphCacheParams();
      GlyphCacheParams(string const & unicodeBlockFile,
                       string const & whiteListFile,
                       string const & blackListFile,
                       size_t glyphCacheMemoryLimit,
                       size_t glyphCacheCount,
                       size_t renderThreadCount);
    };

    struct Params
    {
      RtFormat m_rtFormat;
      bool m_isMergeable;
      bool m_useVA;

      size_t m_videoMemoryLimit;

      /// storages params

      StoragePoolParams m_primaryStoragesParams;
      StoragePoolParams m_smallStoragesParams;
      StoragePoolParams m_blitStoragesParams;
      StoragePoolParams m_multiBlitStoragesParams;
      StoragePoolParams m_tinyStoragesParams;

      /// textures params

      TexturePoolParams m_primaryTexturesParams;
      TexturePoolParams m_fontTexturesParams;
      TexturePoolParams m_renderTargetTexturesParams;
      TexturePoolParams m_styleCacheTexturesParams;

      /// glyph caches params

      GlyphCacheParams m_glyphCacheParams;

      Params();

      void scaleMemoryUsage(double k);
      void fitIntoLimits();
    };

  private:

    typedef map<string, shared_ptr<gl::BaseTexture> > TStaticTextures;

    TStaticTextures m_staticTextures;

    threads::Mutex m_mutex;

    auto_ptr<TTexturePool> m_primaryTextures;
    auto_ptr<TTexturePool> m_fontTextures;
    auto_ptr<TTexturePool> m_styleCacheTextures;
    auto_ptr<TTexturePool> m_renderTargets;

    auto_ptr<TStoragePool> m_primaryStorages;
    auto_ptr<TStoragePool> m_smallStorages;
    auto_ptr<TStoragePool> m_blitStorages;
    auto_ptr<TStoragePool> m_multiBlitStorages;
    auto_ptr<TStoragePool> m_tinyStorages;

    vector<GlyphCache> m_glyphCaches;

    Params m_params;

  public:

    ResourceManager(Params const & p);

    void initGlyphCaches(GlyphCacheParams const & p);

    void initPrimaryStorage(StoragePoolParams const & p);
    void initSmallStorage(StoragePoolParams const & p);
    void initBlitStorage(StoragePoolParams const & p);
    void initMultiBlitStorage(StoragePoolParams const & p);
    void initTinyStorage(StoragePoolParams const & p);

    TStoragePool * primaryStorages();
    TStoragePool * smallStorages();
    TStoragePool * blitStorages();
    TStoragePool * multiBlitStorages();
    TStoragePool * tinyStorages();

    void initPrimaryTextures(TexturePoolParams const & p);
    void initFontTextures(TexturePoolParams const & p);
    void initRenderTargetTextures(TexturePoolParams const & p);
    void initStyleCacheTextures(TexturePoolParams const & p);

    TTexturePool * primaryTextures();
    TTexturePool * fontTextures();
    TTexturePool * renderTargetTextures();
    TTexturePool * styleCacheTextures();

    shared_ptr<gl::BaseTexture> const & getTexture(string const & fileName);

    Params const & params() const;

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

