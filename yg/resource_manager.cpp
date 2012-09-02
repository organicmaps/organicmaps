
#include "internal/opengl.hpp"
#include "base_texture.hpp"
#include "data_traits.hpp"
#include "resource_manager.hpp"
#include "skin_loader.hpp"
#include "storage.hpp"
#include "texture.hpp"
#include "buffer_object.hpp"

#include "../coding/file_reader.hpp"
#include "../coding/parse_xml.hpp"

#include "../base/logging.hpp"
#include "../base/exception.hpp"

namespace yg
{
  typedef gl::Texture<DATA_TRAITS, true> TDynamicTexture;
  typedef gl::Texture<DATA_TRAITS, false> TStaticTexture;

  DECLARE_EXCEPTION(ResourceManagerException, RootException);

  size_t pixelSize(yg::DataFormat texFormat)
  {
    switch (texFormat)
    {
    case yg::Data8Bpp:
      return 4;
    case yg::Data4Bpp:
      return 2;
    case yg::Data565Bpp:
      return 2;
    default:
      return 0;
    }
  }

  TTextureFactory::TTextureFactory(size_t w, size_t h, yg::DataFormat format, char const * resName, size_t batchSize)
    : BasePoolElemFactory(resName, w * h * pixelSize(format), batchSize),
      m_w(w), m_h(h), m_format(format)
  {}

  shared_ptr<gl::BaseTexture> const TTextureFactory::Create()
  {
    switch (m_format)
    {
    case yg::Data4Bpp:
      return shared_ptr<gl::BaseTexture>(new gl::Texture<yg::RGBA4Traits, true>(m_w, m_h));
    case yg::Data8Bpp:
      return shared_ptr<gl::BaseTexture>(new gl::Texture<yg::RGBA8Traits, true>(m_w, m_h));
    case yg::Data565Bpp:
      return shared_ptr<gl::BaseTexture>(new gl::Texture<yg::RGB565Traits, true>(m_w, m_h));
    default:
      return shared_ptr<gl::BaseTexture>();
    }
  }

  TStorageFactory::TStorageFactory(size_t vbSize, size_t ibSize, bool useSingleThreadedOGL, char const * resName, size_t batchSize)
    : BasePoolElemFactory(resName, vbSize + ibSize, batchSize),
      m_vbSize(vbSize),
      m_ibSize(ibSize),
      m_useSingleThreadedOGL(useSingleThreadedOGL)
  {}

  gl::Storage const TStorageFactory::Create()
  {
    gl::Storage res(m_vbSize, m_ibSize);

    if (m_useSingleThreadedOGL)
    {
      res.m_indices->lock();
      res.m_vertices->lock();
    }

    return res;
  }

  void TStorageFactory::BeforeMerge(gl::Storage const & e)
  {
    if (m_useSingleThreadedOGL)
    {
      if (e.m_indices->isLocked())
        e.m_indices->unlock();

      e.m_indices->lock();

      if (e.m_vertices->isLocked())
        e.m_vertices->unlock();

      e.m_vertices->lock();
    }
  }

  ResourceManager::StoragePoolParams::StoragePoolParams(string const & poolName)
    : m_vbSize(0),
      m_vertexSize(0),
      m_ibSize(0),
      m_indexSize(0),
      m_storagesCount(0),
      m_isFixedBufferSize(true),
      m_isFixedBufferCount(true),
      m_scalePriority(0),
      m_poolName(poolName),
      m_isDebugging(false),
      m_allocateOnDemand(false)
  {}

  ResourceManager::StoragePoolParams::StoragePoolParams(size_t vbSize,
                                                        size_t vertexSize,
                                                        size_t ibSize,
                                                        size_t indexSize,
                                                        size_t storagesCount,
                                                        bool isFixedBufferSize,
                                                        bool isFixedBufferCount,
                                                        int scalePriority,
                                                        string const & poolName,
                                                        bool isDebugging,
                                                        bool allocateOnDemand)
    : m_vbSize(vbSize),
      m_vertexSize(vertexSize),
      m_ibSize(ibSize),
      m_indexSize(indexSize),
      m_storagesCount(storagesCount),
      m_isFixedBufferSize(isFixedBufferSize),
      m_isFixedBufferCount(isFixedBufferCount),
      m_scalePriority(scalePriority),
      m_poolName(poolName),
      m_isDebugging(isDebugging),
      m_allocateOnDemand(allocateOnDemand)
  {}

  bool ResourceManager::StoragePoolParams::isFixed() const
  {
    return m_isFixedBufferSize && m_isFixedBufferCount;
  }

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
#ifndef OMIM_PRODUCTION
    int oldMemoryUsage = memoryUsage();
    int oldVBSize = m_vbSize;
    int oldIBSize = m_ibSize;
    int oldStoragesCount = m_storagesCount;
#endif

    if (!m_isFixedBufferSize)
    {
      m_vbSize *= k;
      m_ibSize *= k;
      k = 1;
    }

    if (!m_isFixedBufferCount)
      m_storagesCount *= k;

#ifndef OMIM_PRODUCTION
    LOG(LINFO, ("resizing", m_poolName));
    LOG(LINFO, ("from:", oldVBSize / m_vertexSize, "vertices,", oldIBSize / m_indexSize, "indices,", oldStoragesCount, "storages,", oldMemoryUsage, "bytes total"));
    LOG(LINFO, ("to  :", m_vbSize / m_vertexSize, "vertices,", m_ibSize / m_indexSize, "indices,", m_storagesCount, "storages,", memoryUsage(), "bytes total"));
#endif
  }

  void ResourceManager::StoragePoolParams::distributeFreeMemory(int freeVideoMemory)
  {
    if (!isFixed())
    {
      double k = (freeVideoMemory * m_scaleFactor + memoryUsage()) / memoryUsage();
      scaleMemoryUsage(k);
    }
    else
    {
#ifndef OMIM_PRODUCTION
      if (m_vbSize && m_vertexSize && m_ibSize && m_indexSize)
        LOG(LINFO, (m_poolName, " : ", m_vbSize / m_vertexSize, " vertices, ", m_ibSize / m_indexSize, " indices, ", m_storagesCount, " storages, ", memoryUsage(), " bytes total"));
#endif
    }
  }

  ResourceManager::TexturePoolParams::TexturePoolParams(string const & poolName)
    : m_texWidth(0),
      m_texHeight(0),
      m_texCount(0),
      m_format(yg::Data8Bpp),
      m_isWidthFixed(true),
      m_isHeightFixed(true),
      m_isCountFixed(true),
      m_scalePriority(0),
      m_poolName(poolName),
      m_isDebugging(false),
      m_allocateOnDemand(false)
  {}

  ResourceManager::TexturePoolParams::TexturePoolParams(size_t texWidth,
                                                        size_t texHeight,
                                                        size_t texCount,
                                                        yg::DataFormat format,
                                                        bool isWidthFixed,
                                                        bool isHeightFixed,
                                                        bool isCountFixed,
                                                        int scalePriority,
                                                        string const & poolName,
                                                        bool isDebugging,
                                                        bool allocateOnDemand)
    : m_texWidth(texWidth),
      m_texHeight(texHeight),
      m_texCount(texCount),
      m_format(format),
      m_isWidthFixed(isWidthFixed),
      m_isHeightFixed(isHeightFixed),
      m_isCountFixed(isCountFixed),
      m_scalePriority(scalePriority),
      m_poolName(poolName),
      m_isDebugging(isDebugging),
      m_allocateOnDemand(allocateOnDemand)
  {}

  bool ResourceManager::TexturePoolParams::isFixed() const
  {
    return m_isWidthFixed && m_isHeightFixed && m_isCountFixed;
  }

  bool ResourceManager::TexturePoolParams::isValid() const
  {
    return m_texWidth && m_texHeight && m_texCount;
  }

  size_t ResourceManager::TexturePoolParams::memoryUsage() const
  {
    size_t pixelSize = 0;
    switch (m_format)
    {
    case yg::Data4Bpp:
      pixelSize = 2;
      break;
    case yg::Data8Bpp:
      pixelSize = 4;
      break;
    case yg::Data565Bpp:
      pixelSize = 2;
      break;
    }

    return m_texWidth * m_texHeight * pixelSize * m_texCount;
  }

  void ResourceManager::TexturePoolParams::distributeFreeMemory(int freeVideoMemory)
  {
    if (!isFixed())
    {
      double k = (freeVideoMemory * m_scaleFactor + memoryUsage()) / memoryUsage();
      scaleMemoryUsage(k);
    }
    else
    {
#ifndef OMIM_PRODUCTION
      if (m_texWidth && m_texHeight && m_texCount)
        LOG(LINFO, (m_poolName, " : ", m_texWidth, "x", m_texHeight, ", ", m_texCount, " textures, ", memoryUsage(), " bytes total"));
#endif
    }
  }

  void ResourceManager::TexturePoolParams::scaleMemoryUsage(double k)
  {
#ifndef OMIM_PRODUCTION
    int oldTexCount = m_texCount;
    int oldTexWidth = m_texWidth;
    int oldTexHeight = m_texHeight;
    int oldMemoryUsage = memoryUsage();
#endif

    if (!m_isWidthFixed || !m_isHeightFixed)
    {
      if (k > 1)
      {
        while (k > 2)
        {
          if ((k > 2) && (!m_isWidthFixed))
          {
            m_texWidth *= 2;
            k /= 2;
          }

          if ((k > 2) && (!m_isHeightFixed))
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
            if ((k < 0.5) && (!m_isWidthFixed))
            {
              m_texHeight /= 2;
              k *= 2;
            }

            if ((k < 0.5) && (!m_isHeightFixed))
            {
              m_texWidth /= 2;
              k *= 2;
            }
          }
        }
      }
    }

    if (!m_isCountFixed)
      m_texCount *= k;
#ifndef OMIM_PRODUCTION
    LOG(LINFO, ("scaling memory usage for ", m_poolName));
    LOG(LINFO, (" from : ", oldTexWidth, "x", oldTexHeight, ", ", oldTexCount, " textures, ", oldMemoryUsage, " bytes total"));
    LOG(LINFO, (" to   : ", m_texWidth, "x", m_texHeight, ", ", m_texCount, " textures, ", memoryUsage(), " bytes total"));
#endif
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
                                                      size_t renderThreadCount,
                                                      bool * debuggingFlags)
    : m_unicodeBlockFile(unicodeBlockFile),
      m_whiteListFile(whiteListFile),
      m_blackListFile(blackListFile),
      m_glyphCacheMemoryLimit(glyphCacheMemoryLimit),
      m_glyphCacheCount(glyphCacheCount),
      m_renderThreadCount(renderThreadCount)
  {
    if (debuggingFlags)
      copy(debuggingFlags, debuggingFlags + glyphCacheCount, back_inserter(m_debuggingFlags));
    else
      for (unsigned i = 0; i < glyphCacheCount; ++i)
        m_debuggingFlags.push_back(false);
  }

namespace
{
  void GetGLStringSafe(GLenum name, string & str)
  {
    char const * s = reinterpret_cast<char const*>(glGetString(name));
    if (s)
      str = s;
    else
    {
      OGLCHECKAFTER;
      LOG(LWARNING, ("Can't get OpenGL name"));
    }
  }
}

  ResourceManager::Params::Params()
    : m_rtFormat(yg::Data8Bpp),
      m_texFormat(yg::Data4Bpp),
      m_texRtFormat(yg::Data4Bpp),
      m_useSingleThreadedOGL(false),
      m_videoMemoryLimit(0),
      m_primaryStoragesParams("primaryStorage"),
      m_smallStoragesParams("smallStorage"),
      m_blitStoragesParams("blitStorage"),
      m_multiBlitStoragesParams("multiBlitStorage"),
      m_guiThreadStoragesParams("tinyStorage"),
      m_primaryTexturesParams("primaryTexture"),
      m_fontTexturesParams("fontTexture"),
      m_renderTargetTexturesParams("renderTargetTexture"),
      m_styleCacheTexturesParams("styleCacheTexture"),
      m_guiThreadTexturesParams("guiThreadTexture")
  {
    GetGLStringSafe(GL_VENDOR, m_vendorName);
    GetGLStringSafe(GL_RENDERER, m_rendererName);
  }

  bool ResourceManager::Params::isGPU(char const * vendorName, char const * rendererName, bool strictMatch) const
  {
    if (strictMatch)
    {
      return (m_vendorName == string(vendorName))
          && (m_rendererName == string(rendererName));
    }
    else
      return (m_vendorName.find(vendorName) != string::npos)
          && (m_rendererName.find(rendererName) != string::npos);
  }

  void ResourceManager::Params::checkDeviceCaps()
  {
    /// general case
    m_texRtFormat = yg::Data4Bpp;
    m_useReadPixelsToSynchronize = false;

    if (isGPU("Vivante Corporation", "GC800 core", true))
    {
#ifndef __mips__
      /// glMapBuffer doesn't work on this GPU on non-MIPS devices,
      /// so we're switching to glBufferSubData.
      yg::gl::g_isMapBufferSupported = false;
#endif
    }

    if (isGPU("Qualcomm", "Adreno", false))
      m_texRtFormat = yg::Data8Bpp;

    if (isGPU("Samsung Electronics", "FIMG-3DSE", false))
      m_texRtFormat = yg::Data8Bpp;

    if (isGPU("Broadcom", "VideoCore IV HW", false))
      m_texRtFormat = yg::Data8Bpp;

    if (isGPU("Imagination Technologies", "PowerVR MBX", false))
    {
      m_rtFormat = yg::Data8Bpp;
      m_texRtFormat = yg::Data8Bpp;
    }

    bool isAndroidDevice = GetPlatform().DeviceName() == "Android";

    /// on PowerVR chips on Android glFinish doesn't work, so we should use
    /// glReadPixels instead of glFinish to synchronize.
    if (isGPU("Imagination Technologies", "PowerVR", false) && isAndroidDevice)
      m_useReadPixelsToSynchronize = true;

    LOG(LINFO, ("selected", yg::formatName(m_texRtFormat), "format for tile textures"));

    if (m_useReadPixelsToSynchronize)
      LOG(LINFO, ("using ReadPixels instead of glFinish to synchronize"));
  }

  void ResourceManager::Params::fitIntoLimits()
  {
    initScaleWeights();

    int oldMemoryUsage = memoryUsage();

    int videoMemoryLimit = m_videoMemoryLimit;

    if (videoMemoryLimit == 0)
    {
      videoMemoryLimit = memoryUsage();
      LOG(LINFO, ("videoMemoryLimit is not set. will not scale resource usage."));
    }

    if (videoMemoryLimit < fixedMemoryUsage())
    {
      LOG(LINFO, ("videoMemoryLimit", videoMemoryLimit, "is less than an amount of fixed resources", fixedMemoryUsage()));
      videoMemoryLimit = memoryUsage();
    }

    if (videoMemoryLimit < memoryUsage())
    {
      LOG(LINFO, ("videoMemoryLimit", videoMemoryLimit, "is less than amount of currently allocated resources", memoryUsage()));
      videoMemoryLimit = memoryUsage();
    }

    int freeVideoMemory = videoMemoryLimit - oldMemoryUsage;

    /// distributing free memory according to the weights
    distributeFreeMemory(freeVideoMemory);

    LOG(LINFO, ("resizing from", oldMemoryUsage, "bytes to", memoryUsage(), "bytes of video memory"));
  }

  int ResourceManager::Params::memoryUsage() const
  {
    return m_primaryStoragesParams.memoryUsage()
        + m_smallStoragesParams.memoryUsage()
        + m_blitStoragesParams.memoryUsage()
        + m_multiBlitStoragesParams.memoryUsage()
        + m_guiThreadStoragesParams.memoryUsage()
        + m_primaryTexturesParams.memoryUsage()
        + m_fontTexturesParams.memoryUsage()
        + m_renderTargetTexturesParams.memoryUsage()
        + m_styleCacheTexturesParams.memoryUsage()
        + m_guiThreadTexturesParams.memoryUsage();
  }

  int ResourceManager::Params::fixedMemoryUsage() const
  {
    return (m_primaryStoragesParams.isFixed() ? m_primaryStoragesParams.memoryUsage() : 0)
      + (m_smallStoragesParams.isFixed() ? m_smallStoragesParams.memoryUsage() : 0)
      + (m_blitStoragesParams.isFixed() ? m_blitStoragesParams.memoryUsage() : 0)
      + (m_multiBlitStoragesParams.isFixed() ? m_multiBlitStoragesParams.memoryUsage() : 0)
      + (m_guiThreadStoragesParams.isFixed() ? m_guiThreadStoragesParams.memoryUsage() : 0)
      + (m_primaryTexturesParams.isFixed() ? m_primaryTexturesParams.memoryUsage() : 0)
      + (m_fontTexturesParams.isFixed() ? m_fontTexturesParams.memoryUsage() : 0)
      + (m_renderTargetTexturesParams.isFixed() ? m_renderTargetTexturesParams.memoryUsage() : 0)
      + (m_styleCacheTexturesParams.isFixed() ? m_styleCacheTexturesParams.memoryUsage() : 0)
      + (m_guiThreadTexturesParams.isFixed() ? m_guiThreadTexturesParams.memoryUsage() : 0);
  }

  void ResourceManager::Params::distributeFreeMemory(int freeVideoMemory)
  {
    m_primaryStoragesParams.distributeFreeMemory(freeVideoMemory);
    m_smallStoragesParams.distributeFreeMemory(freeVideoMemory);
    m_blitStoragesParams.distributeFreeMemory(freeVideoMemory);
    m_multiBlitStoragesParams.distributeFreeMemory(freeVideoMemory);
    m_guiThreadStoragesParams.distributeFreeMemory(freeVideoMemory);

    m_primaryTexturesParams.distributeFreeMemory(freeVideoMemory);
    m_fontTexturesParams.distributeFreeMemory(freeVideoMemory);
    m_renderTargetTexturesParams.distributeFreeMemory(freeVideoMemory);
    m_styleCacheTexturesParams.distributeFreeMemory(freeVideoMemory);
    m_guiThreadTexturesParams.distributeFreeMemory(freeVideoMemory);
  }

  ResourceManager::ResourceManager(Params const & p)
    : m_params(p)
  {
    initGlyphCaches(p.m_glyphCacheParams);

    initStoragePool(p.m_primaryStoragesParams, m_primaryStorages);
    initStoragePool(p.m_smallStoragesParams, m_smallStorages);
    initStoragePool(p.m_blitStoragesParams, m_blitStorages);
    initStoragePool(p.m_multiBlitStoragesParams, m_multiBlitStorages);
    initStoragePool(p.m_guiThreadStoragesParams, m_guiThreadStorages);

    initTexturePool(p.m_primaryTexturesParams, m_primaryTextures);
    initTexturePool(p.m_fontTexturesParams, m_fontTextures);
    initTexturePool(p.m_renderTargetTexturesParams, m_renderTargets);
    initTexturePool(p.m_styleCacheTexturesParams, m_styleCacheTextures);
    initTexturePool(p.m_guiThreadTexturesParams, m_guiThreadTextures);

    if (!yg::gl::g_isBufferObjectsSupported)
      LOG(LINFO, ("buffer objects are unsupported. using client vertex array instead."));
  }

  void ResourceManager::Params::initScaleWeights()
  {
    double prioritySum = (m_primaryStoragesParams.isFixed() ? 0 : m_primaryStoragesParams.m_scalePriority)
        + (m_smallStoragesParams.isFixed() ? 0 : m_smallStoragesParams.m_scalePriority)
        + (m_blitStoragesParams.isFixed() ? 0 : m_blitStoragesParams.m_scalePriority)
        + (m_multiBlitStoragesParams.isFixed() ? 0 : m_multiBlitStoragesParams.m_scalePriority)
        + (m_guiThreadStoragesParams.isFixed() ? 0 : m_guiThreadStoragesParams.m_scalePriority)
        + (m_primaryTexturesParams.isFixed() ? 0 : m_primaryTexturesParams.m_scalePriority)
        + (m_fontTexturesParams.isFixed() ? 0 : m_fontTexturesParams.m_scalePriority)
        + (m_renderTargetTexturesParams.isFixed() ? 0 : m_renderTargetTexturesParams.m_scalePriority)
        + (m_styleCacheTexturesParams.isFixed() ? 0 : m_styleCacheTexturesParams.m_scalePriority)
        + (m_guiThreadTexturesParams.isFixed() ? 0 : m_guiThreadTexturesParams.m_scalePriority);

    if (prioritySum == 0)
      return;

    m_primaryStoragesParams.m_scaleFactor = m_primaryStoragesParams.m_scalePriority / prioritySum;
    m_smallStoragesParams.m_scaleFactor = m_smallStoragesParams.m_scalePriority / prioritySum;
    m_blitStoragesParams.m_scaleFactor = m_blitStoragesParams.m_scalePriority / prioritySum;
    m_multiBlitStoragesParams.m_scaleFactor = m_multiBlitStoragesParams.m_scalePriority / prioritySum;
    m_guiThreadStoragesParams.m_scaleFactor = m_guiThreadStoragesParams.m_scalePriority / prioritySum;
    m_primaryTexturesParams.m_scaleFactor = m_primaryTexturesParams.m_scalePriority / prioritySum;
    m_fontTexturesParams.m_scaleFactor = m_fontTexturesParams.m_scalePriority / prioritySum;
    m_renderTargetTexturesParams.m_scaleFactor = m_renderTargetTexturesParams.m_scalePriority / prioritySum;
    m_styleCacheTexturesParams.m_scaleFactor = m_styleCacheTexturesParams.m_scalePriority / prioritySum;
    m_guiThreadTexturesParams.m_scaleFactor = m_guiThreadTexturesParams.m_scalePriority / prioritySum;
  }

  void ResourceManager::initGlyphCaches(GlyphCacheParams const & p)
  {
    if (p.m_glyphCacheMemoryLimit && p.m_glyphCacheCount)
    {
      LOG(LDEBUG, ("allocating ", p.m_glyphCacheCount, " glyphCaches, ", p.m_glyphCacheMemoryLimit, " bytes total."));

      for (size_t i = 0; i < p.m_glyphCacheCount; ++i)
        m_glyphCaches.push_back(GlyphCache(GlyphCache::Params(p.m_unicodeBlockFile, p.m_whiteListFile, p.m_blackListFile, p.m_glyphCacheMemoryLimit / p.m_glyphCacheCount, p.m_debuggingFlags[i])));
    }
    else
      LOG(LERROR, ("no params to init glyph caches."));
  }

  void ResourceManager::initStoragePool(StoragePoolParams const & p, scoped_ptr<TStoragePool> & pool)
  {
    if (p.isValid())
    {
      LOG(LINFO, ("initializing", p.m_poolName, "resource pool. vbSize=", p.m_vbSize, ", ibSize=", p.m_ibSize));
      TStorageFactory storageFactory(p.m_vbSize, p.m_ibSize, m_params.m_useSingleThreadedOGL, p.m_poolName.c_str(), p.m_allocateOnDemand ? p.m_storagesCount : 0);

      if (m_params.m_useSingleThreadedOGL)
      {
        if (p.m_allocateOnDemand)
          pool.reset(new TOnDemandSingleThreadedStoragePoolImpl(new TOnDemandSingleThreadedStoragePoolTraits(storageFactory, p.m_storagesCount)));
        else
          pool.reset(new TFixedSizeMergeableStoragePoolImpl(new TFixedSizeMergeableStoragePoolTraits(storageFactory, p.m_storagesCount)));
      }
      else
      {
        if (p.m_allocateOnDemand)
          pool.reset(new TOnDemandMultiThreadedStoragePoolImpl(new TOnDemandMultiThreadedStoragePoolTraits(storageFactory, p.m_storagesCount)));
        else
          pool.reset(new TFixedSizeNonMergeableStoragePoolImpl(new TFixedSizeNonMergeableStoragePoolTraits(storageFactory, p.m_storagesCount)));
      }

      pool->SetIsDebugging(p.m_isDebugging);
    }
    else
      LOG(LINFO, ("no ", p.m_poolName, " resource"));
  }

  void ResourceManager::initTexturePool(TexturePoolParams const & p, scoped_ptr<TTexturePool> & pool)
  {
    if (p.isValid())
    {
      TTextureFactory textureFactory(p.m_texWidth, p.m_texHeight, p.m_format, p.m_poolName.c_str(), p.m_allocateOnDemand ? p.m_texCount : 0);

      if (m_params.m_useSingleThreadedOGL)
      {
        if (p.m_allocateOnDemand)
          pool.reset(new TOnDemandSingleThreadedTexturePoolImpl(new TOnDemandSingleThreadedTexturePoolTraits(textureFactory, p.m_texCount)));
        else
          pool.reset(new TFixedSizeTexturePoolImpl(new TFixedSizeTexturePoolTraits(textureFactory, p.m_texCount)));
      }
      else
      {
        if (p.m_allocateOnDemand)
          pool.reset(new TOnDemandMultiThreadedTexturePoolImpl(new TOnDemandMultiThreadedTexturePoolTraits(textureFactory, p.m_texCount)));
        else
          pool.reset(new TFixedSizeTexturePoolImpl(new TFixedSizeTexturePoolTraits(textureFactory, p.m_texCount)));
      }

      pool->SetIsDebugging(p.m_isDebugging);
    }
    else
      LOG(LINFO, ("no ", p.m_poolName, " resource"));
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

    if (m_guiThreadStorages.get())
      m_guiThreadStorages->EnterBackground();

    if (m_primaryTextures.get())
      m_primaryTextures->EnterBackground();

    if (m_fontTextures.get())
      m_fontTextures->EnterBackground();

    if (m_renderTargets.get())
      m_renderTargets->EnterBackground();

    if (m_styleCacheTextures.get())
      m_styleCacheTextures->EnterBackground();

    if (m_guiThreadTextures.get())
      m_guiThreadTextures->EnterBackground();
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

    if (m_guiThreadStorages.get())
      m_guiThreadStorages->EnterForeground();

    if (m_primaryTextures.get())
      m_primaryTextures->EnterForeground();

    if (m_fontTextures.get())
      m_fontTextures->EnterForeground();

    if (m_renderTargets.get())
      m_renderTargets->EnterForeground();

    if (m_styleCacheTextures.get())
      m_styleCacheTextures->EnterForeground();

    if (m_guiThreadTextures.get())
      m_guiThreadTextures->EnterForeground();
  }

  shared_ptr<yg::gl::BaseTexture> ResourceManager::createRenderTarget(unsigned w, unsigned h)
  {
    switch (m_params.m_rtFormat)
    {
    case Data8Bpp:
      return make_shared_ptr(new gl::Texture<RGBA8Traits, false>(w, h));
    case Data4Bpp:
      return make_shared_ptr(new gl::Texture<RGBA4Traits, false>(w, h));
    case Data565Bpp:
      return make_shared_ptr(new gl::Texture<RGB565Traits, false>(w, h));
    default:
      MYTHROW(ResourceManagerException, ("unknown render target format"));
    };
  }

  int ResourceManager::renderThreadGlyphCacheID(int threadNum) const
  {
    ASSERT(threadNum < m_params.m_glyphCacheParams.m_renderThreadCount, (threadNum, m_params.m_glyphCacheParams.m_renderThreadCount));
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

  TStoragePool * ResourceManager::primaryStorages()
  {
    return m_primaryStorages.get();
  }

  TStoragePool * ResourceManager::smallStorages()
  {
    return m_smallStorages.get();
  }

  TStoragePool * ResourceManager::guiThreadStorages()
  {
    return m_guiThreadStorages.get();
  }

  TStoragePool * ResourceManager::blitStorages()
  {
    return m_blitStorages.get();
  }

  TStoragePool * ResourceManager::multiBlitStorages()
  {
    return m_multiBlitStorages.get();
  }

  TTexturePool * ResourceManager::primaryTextures()
  {
    return m_primaryTextures.get();
  }

  TTexturePool * ResourceManager::fontTextures()
  {
    return m_fontTextures.get();
  }

  TTexturePool * ResourceManager::renderTargetTextures()
  {
    return m_renderTargets.get();
  }

  TTexturePool * ResourceManager::styleCacheTextures()
  {
    return m_styleCacheTextures.get();
  }

  TTexturePool * ResourceManager::guiThreadTextures()
  {
    return m_guiThreadTextures.get();
  }

  void ResourceManager::updatePoolState()
  {
    if (m_primaryTextures.get())
      m_primaryTextures->UpdateState();
    if (m_fontTextures.get())
      m_fontTextures->UpdateState();
    if (m_styleCacheTextures.get())
      m_styleCacheTextures->UpdateState();
    if (m_renderTargets.get())
      m_renderTargets->UpdateState();
    if (m_guiThreadTextures.get())
      m_guiThreadTextures->UpdateState();

    if (m_guiThreadStorages.get())
      m_guiThreadStorages->UpdateState();
    if (m_primaryStorages.get())
      m_primaryStorages->UpdateState();
    if (m_smallStorages.get())
      m_smallStorages->UpdateState();
    if (m_blitStorages.get())
      m_blitStorages->UpdateState();
    if (m_multiBlitStorages.get())
      m_multiBlitStorages->UpdateState();
  }

  void ResourceManager::cancel()
  {
    if (m_primaryTextures.get())
      m_primaryTextures->Cancel();
    if (m_fontTextures.get())
      m_fontTextures->Cancel();
    if (m_styleCacheTextures.get())
      m_styleCacheTextures->Cancel();
    if (m_renderTargets.get())
      m_renderTargets->Cancel();
    if (m_guiThreadTextures.get())
      m_guiThreadTextures->Cancel();

    if (m_primaryStorages.get())
      m_primaryStorages->Cancel();
    if (m_smallStorages.get())
      m_smallStorages->Cancel();
    if (m_blitStorages.get())
      m_blitStorages->Cancel();
    if (m_multiBlitStorages.get())
      m_multiBlitStorages->Cancel();
    if (m_guiThreadStorages.get())
      m_guiThreadStorages->Cancel();
  }

  bool ResourceManager::useReadPixelsToSynchronize() const
  {
    return m_params.m_useReadPixelsToSynchronize;
  }
}
