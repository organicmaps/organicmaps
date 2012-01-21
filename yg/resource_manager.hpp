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

  enum DataFormat
  {
    Data8Bpp,
    Data4Bpp
  };

  struct TTextureFactory : BasePoolElemFactory
  {
    size_t m_w;
    size_t m_h;
    yg::DataFormat m_format;
    TTextureFactory(size_t w, size_t h, yg::DataFormat format, char const * resName);
    shared_ptr<gl::BaseTexture> const Create();
  };

  struct TStorageFactory : BasePoolElemFactory
  {
    size_t m_vbSize;
    size_t m_ibSize;
    bool m_useVA;
    bool m_useSingleThreadedOGL;
    TStorageFactory(size_t vbSize, size_t ibSize, bool useVA, bool useSingleThreadedOGL, char const * resName);
    gl::Storage const Create();
    void BeforeMerge(gl::Storage const & e);
  };

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

  class ResourceManager
  {
  public:

    struct StoragePoolParams
    {
      size_t m_vbSize;
      size_t m_vertexSize;
      size_t m_ibSize;
      size_t m_indexSize;
      size_t m_storagesCount;

      bool m_isFixedBufferSize;
      bool m_isFixedBufferCount;

      int m_scalePriority;
      double m_scaleFactor;

      string m_poolName;

      bool m_isDebugging;

      StoragePoolParams(size_t vbSize,
                        size_t vertexSize,
                        size_t ibSize,
                        size_t indexSize,
                        size_t storagesCount,
                        bool isFixedBufferSize,
                        bool isFixedBufferCount,
                        int scalePriority,
                        string const & poolName,
                        bool isDebugging = false);

      StoragePoolParams(string const & poolName);

      bool isFixed() const;
      bool isValid() const;
      void scaleMemoryUsage(double k);
      void distributeFreeMemory(int freeVideoMemory);
      size_t memoryUsage() const;
    };

    struct TexturePoolParams
    {
      size_t m_texWidth;
      size_t m_texHeight;
      size_t m_texCount;
      yg::DataFormat m_format;

      bool m_isWidthFixed;
      bool m_isHeightFixed;
      bool m_isCountFixed;

      int m_scalePriority;
      double m_scaleFactor;

      string m_poolName;

      bool m_isDebugging;

      TexturePoolParams(size_t texWidth,
                        size_t texHeight,
                        size_t texCount,
                        yg::DataFormat format,
                        bool isWidthFixed,
                        bool isHeightFixed,
                        bool isCountFixed,
                        int scalePriority,
                        string const & poolName,
                        bool isDebugging = false);

      TexturePoolParams(string const & poolName);

      bool isFixed() const;
      bool isValid() const;
      void scaleMemoryUsage(double k);
      void distributeFreeMemory(int freeVideoMemory);
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

      vector<bool> m_debuggingFlags;

      GlyphCacheParams();
      GlyphCacheParams(string const & unicodeBlockFile,
                       string const & whiteListFile,
                       string const & blackListFile,
                       size_t glyphCacheMemoryLimit,
                       size_t glyphCacheCount,
                       size_t renderThreadCount,
                       bool * debuggingFlags = 0);
    };

    struct Params
    {
      DataFormat m_rtFormat;
      DataFormat m_texFormat;
      bool m_useSingleThreadedOGL;
      bool m_useVA;

      size_t m_videoMemoryLimit;

      /// storages params

      StoragePoolParams m_primaryStoragesParams;
      StoragePoolParams m_smallStoragesParams;
      StoragePoolParams m_blitStoragesParams;
      StoragePoolParams m_multiBlitStoragesParams;
      StoragePoolParams m_guiThreadStoragesParams;

      /// textures params

      TexturePoolParams m_primaryTexturesParams;
      TexturePoolParams m_fontTexturesParams;
      TexturePoolParams m_renderTargetTexturesParams;
      TexturePoolParams m_styleCacheTexturesParams;
      TexturePoolParams m_guiThreadTexturesParams;

      /// glyph caches params

      GlyphCacheParams m_glyphCacheParams;

      Params();

      void distributeFreeMemory(int freeVideoMemory);
      void fitIntoLimits();
      int memoryUsage() const;
      int fixedMemoryUsage() const;
      void initScaleWeights();
    };

  private:

    typedef map<string, shared_ptr<gl::BaseTexture> > TStaticTextures;

    TStaticTextures m_staticTextures;

    threads::Mutex m_mutex;

    auto_ptr<TTexturePool> m_primaryTextures;
    auto_ptr<TTexturePool> m_fontTextures;
    auto_ptr<TTexturePool> m_styleCacheTextures;
    auto_ptr<TTexturePool> m_renderTargets;
    auto_ptr<TTexturePool> m_guiThreadTextures;

    auto_ptr<TStoragePool> m_primaryStorages;
    auto_ptr<TStoragePool> m_smallStorages;
    auto_ptr<TStoragePool> m_blitStorages;
    auto_ptr<TStoragePool> m_multiBlitStorages;
    auto_ptr<TStoragePool> m_guiThreadStorages;

    vector<GlyphCache> m_glyphCaches;

    Params m_params;

  public:

    ResourceManager(Params const & p);

    void initGlyphCaches(GlyphCacheParams const & p);

    void initStoragePool(StoragePoolParams const & p, auto_ptr<TStoragePool> & pool);

    TStoragePool * primaryStorages();
    TStoragePool * smallStorages();
    TStoragePool * blitStorages();
    TStoragePool * multiBlitStorages();
    TStoragePool * guiThreadStorages();

    void initTexturePool(TexturePoolParams const & p, auto_ptr<TTexturePool> & pool);

    TTexturePool * primaryTextures();
    TTexturePool * fontTextures();
    TTexturePool * renderTargetTextures();
    TTexturePool * styleCacheTextures();
    TTexturePool * guiThreadTextures();

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

    void updatePoolState();

    void cancel();

    shared_ptr<yg::gl::BaseTexture> createRenderTarget(unsigned w, unsigned h);
  };

  Skin * loadSkin(shared_ptr<ResourceManager> const & resourceManager,
                  string const & fileName,
                  size_t dynamicPagesCount,
                  size_t textPagesCount);
}

