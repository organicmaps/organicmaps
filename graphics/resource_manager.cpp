#include "resource_manager.hpp"

#include "opengl/opengl.hpp"
#include "opengl/base_texture.hpp"
#include "opengl/data_traits.hpp"
#include "opengl/storage.hpp"
#include "opengl/texture.hpp"
#include "opengl/buffer_object.hpp"

#include "skin_loader.hpp"

#include "../coding/file_reader.hpp"
#include "../coding/parse_xml.hpp"

#include "../base/logging.hpp"
#include "../base/exception.hpp"

namespace graphics
{
  typedef gl::Texture<DATA_TRAITS, true> TDynamicTexture;
  typedef gl::Texture<DATA_TRAITS, false> TStaticTexture;

  DECLARE_EXCEPTION(ResourceManagerException, RootException);

  size_t pixelSize(graphics::DataFormat texFormat)
  {
    switch (texFormat)
    {
    case graphics::Data8Bpp:
      return 4;
    case graphics::Data4Bpp:
      return 2;
    case graphics::Data565Bpp:
      return 2;
    default:
      return 0;
    }
  }

  TTextureFactory::TTextureFactory(size_t w, size_t h, graphics::DataFormat format, char const * resName, size_t batchSize)
    : BasePoolElemFactory(resName, w * h * pixelSize(format), batchSize),
      m_w(w), m_h(h), m_format(format)
  {}

  shared_ptr<gl::BaseTexture> const TTextureFactory::Create()
  {
    switch (m_format)
    {
    case graphics::Data4Bpp:
      return shared_ptr<gl::BaseTexture>(new gl::Texture<graphics::RGBA4Traits, true>(m_w, m_h));
    case graphics::Data8Bpp:
      return shared_ptr<gl::BaseTexture>(new gl::Texture<graphics::RGBA8Traits, true>(m_w, m_h));
    case graphics::Data565Bpp:
      return shared_ptr<gl::BaseTexture>(new gl::Texture<graphics::RGB565Traits, true>(m_w, m_h));
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

  ResourceManager::StoragePoolParams::StoragePoolParams(size_t vbSize,
                                                        size_t vertexSize,
                                                        size_t ibSize,
                                                        size_t indexSize,
                                                        size_t storagesCount,
                                                        EStorageType storageType,
                                                        bool isDebugging)
    : m_vbSize(vbSize),
      m_vertexSize(vertexSize),
      m_ibSize(ibSize),
      m_indexSize(indexSize),
      m_storagesCount(storagesCount),
      m_storageType(storageType),
      m_isDebugging(isDebugging)
  {}

  ResourceManager::StoragePoolParams::StoragePoolParams(EStorageType storageType)
    : m_vbSize(0),
      m_vertexSize(0),
      m_ibSize(0),
      m_indexSize(0),
      m_storagesCount(0),
      m_storageType(storageType),
      m_isDebugging(false)
  {}

  ResourceManager::StoragePoolParams::StoragePoolParams()
    : m_vbSize(0),
      m_vertexSize(0),
      m_ibSize(0),
      m_indexSize(0),
      m_storagesCount(0),
      m_storageType(EInvalidStorage),
      m_isDebugging(false)
  {}

  bool ResourceManager::StoragePoolParams::isValid() const
  {
    return m_vbSize && m_ibSize && m_storagesCount;
  }

  ResourceManager::TexturePoolParams::TexturePoolParams(size_t texWidth,
                                                        size_t texHeight,
                                                        size_t texCount,
                                                        graphics::DataFormat format,
                                                        ETextureType textureType,
                                                        bool isDebugging)
    : m_texWidth(texWidth),
      m_texHeight(texHeight),
      m_texCount(texCount),
      m_format(format),
      m_textureType(textureType),
      m_isDebugging(isDebugging)
  {}

  ResourceManager::TexturePoolParams::TexturePoolParams(ETextureType textureType)
    : m_texWidth(0),
      m_texHeight(0),
      m_texCount(0),
      m_textureType(textureType),
      m_isDebugging(false)
  {}

  ResourceManager::TexturePoolParams::TexturePoolParams()
    : m_texWidth(0),
      m_texHeight(0),
      m_texCount(0),
      m_textureType(EInvalidTexture),
      m_isDebugging(false)
  {}

  bool ResourceManager::TexturePoolParams::isValid() const
  {
    return m_texWidth && m_texHeight && m_texCount;
  }

  ResourceManager::GlyphCacheParams::GlyphCacheParams()
    : m_glyphCacheMemoryLimit(0)
  {}

  ResourceManager::GlyphCacheParams::GlyphCacheParams(string const & unicodeBlockFile,
                                                      string const & whiteListFile,
                                                      string const & blackListFile,
                                                      size_t glyphCacheMemoryLimit,
                                                      EDensity density)
    : m_unicodeBlockFile(unicodeBlockFile),
      m_whiteListFile(whiteListFile),
      m_blackListFile(blackListFile),
      m_glyphCacheMemoryLimit(glyphCacheMemoryLimit),
      m_density(density)
  {}

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
    : m_rtFormat(graphics::Data8Bpp),
      m_texFormat(graphics::Data4Bpp),
      m_texRtFormat(graphics::Data4Bpp),
      m_useSingleThreadedOGL(false),
      m_videoMemoryLimit(0),
      m_renderThreadsCount(0),
      m_threadSlotsCount(0)
  {
    for (unsigned i = 0; i < EInvalidStorage + 1; ++i)
      m_storageParams.push_back(ResourceManager::StoragePoolParams(EStorageType(i)));

    for (unsigned i = 0; i < EInvalidTexture + 1; ++i)
      m_textureParams.push_back(ResourceManager::TexturePoolParams(ETextureType(i)));

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
    m_texRtFormat = graphics::Data4Bpp;
    m_useReadPixelsToSynchronize = false;

    if (isGPU("Vivante Corporation", "GC800 core", true))
    {
#ifndef __mips__
      /// glMapBuffer doesn't work on this GPU on non-MIPS devices,
      /// so we're switching to glBufferSubData.
      graphics::gl::g_isMapBufferSupported = false;
#endif
    }

    if (isGPU("Qualcomm", "Adreno", false))
      m_texRtFormat = graphics::Data8Bpp;

    if (isGPU("Samsung Electronics", "FIMG-3DSE", false))
      m_texRtFormat = graphics::Data8Bpp;

    if (isGPU("Broadcom", "VideoCore IV HW", false))
      m_texRtFormat = graphics::Data8Bpp;

    if (isGPU("Imagination Technologies", "PowerVR MBX", false))
    {
      m_rtFormat = graphics::Data8Bpp;
      m_texRtFormat = graphics::Data8Bpp;
    }

#ifdef OMIM_OS_ANDROID
    // on PowerVR chips on Android glFinish doesn't work, so we should use
    // glReadPixels instead of glFinish to synchronize.
    if (isGPU("Imagination Technologies", "PowerVR SGX 540", false))
      m_useReadPixelsToSynchronize = true;
#endif

    LOG(LINFO, ("selected", graphics::formatName(m_texRtFormat), "format for tile textures"));

    if (m_useReadPixelsToSynchronize)
      LOG(LINFO, ("using ReadPixels instead of glFinish to synchronize"));
  }

  void ResourceManager::loadSkinInfoAndTexture(string const & skinFileName, EDensity density)
  {
    try
    {
      ReaderPtr<Reader> reader(GetPlatform().GetReader(resourcePath(skinFileName, density)));
      reader.ReadAsString(m_skinBuffer);

      size_t i = m_skinBuffer.find("file=\"", 0);
      if (i == string::npos)
        MYTHROW(RootException, ("Invalid skin file"));
      i += strlen("file=\"");

      size_t const j = m_skinBuffer.find('\"', i);
      if (j == string::npos)
        MYTHROW(RootException, ("Invalid skin file"));

      string const textureFileName = m_skinBuffer.substr(i, j-i);
      m_staticTextures[textureFileName] = make_shared_ptr(new TStaticTexture(textureFileName, density));
    }
    catch (RootException const & ex)
    {
      LOG(LERROR, ("Error reading skin file", skinFileName, ex.Msg()));
    }
  }

  ResourceManager::ResourceManager(Params const & p, string const & skinFileName, EDensity density)
    : m_params(p)
  {
    // Load skin and texture once before thread's initializing.
    loadSkinInfoAndTexture(skinFileName, density);

    initThreadSlots(p);

    m_storagePools.resize(EInvalidStorage + 1);

    for (unsigned i = 0; i < EInvalidStorage + 1; ++i)
      initStoragePool(p.m_storageParams[i], m_storagePools[i]);

    m_texturePools.resize(EInvalidTexture + 1);

    for (unsigned i = 0; i < EInvalidTexture + 1; ++i)
      initTexturePool(p.m_textureParams[i], m_texturePools[i]);

    if (!graphics::gl::g_isBufferObjectsSupported)
      LOG(LINFO, ("buffer objects are unsupported. using client vertex array instead."));
  }

  void ResourceManager::initThreadSlots(Params const & p)
  {
    LOG(LDEBUG, ("allocating ", p.m_threadSlotsCount, " threads slots"));
    LOG(LDEBUG, ("glyphCacheMemoryLimit is ", p.m_glyphCacheParams.m_glyphCacheMemoryLimit, " bytes total."));

    m_threadSlots.resize(p.m_threadSlotsCount);

    for (unsigned i = 0; i < p.m_threadSlotsCount; ++i)
    {
      GlyphCacheParams gccp = p.m_glyphCacheParams;

      GlyphCache::Params gcp(gccp.m_unicodeBlockFile,
                             gccp.m_whiteListFile,
                             gccp.m_blackListFile,
                             gccp.m_glyphCacheMemoryLimit / p.m_threadSlotsCount,
                             gccp.m_density,
                             false);

      m_threadSlots[i].m_glyphCache.reset(new GlyphCache(gcp));

      if (p.m_useSingleThreadedOGL)
      {
        if (i == 0)
          m_threadSlots[i].m_programManager.reset(new gl::ProgramManager());
        else
          m_threadSlots[i].m_programManager = m_threadSlots[0].m_programManager;
      }
      else
        m_threadSlots[i].m_programManager.reset(new gl::ProgramManager());
    }
  }

  void ResourceManager::initStoragePool(StoragePoolParams const & p, shared_ptr<TStoragePool> & pool)
  {
    if (p.isValid())
    {
      LOG(LINFO, ("initializing", convert(p.m_storageType), "resource pool. vbSize=", p.m_vbSize, ", ibSize=", p.m_ibSize));
      TStorageFactory storageFactory(p.m_vbSize,
                                     p.m_ibSize,
                                     m_params.m_useSingleThreadedOGL,
                                     convert(p.m_storageType),
                                     p.m_storagesCount);

      if (m_params.m_useSingleThreadedOGL)
        pool.reset(new TOnDemandSingleThreadedStoragePoolImpl(new TOnDemandSingleThreadedStoragePoolTraits(storageFactory, p.m_storagesCount)));
      else
        pool.reset(new TOnDemandMultiThreadedStoragePoolImpl(new TOnDemandMultiThreadedStoragePoolTraits(storageFactory, p.m_storagesCount)));

      pool->SetIsDebugging(p.m_isDebugging);
    }
    else
      LOG(LINFO, ("no ", convert(p.m_storageType), " resource"));
  }

  TStoragePool * ResourceManager::storagePool(EStorageType type)
  {
    return m_storagePools[type].get();
  }

  void ResourceManager::initTexturePool(TexturePoolParams const & p,
                                        shared_ptr<TTexturePool> & pool)
  {
    if (p.isValid())
    {
      TTextureFactory textureFactory(p.m_texWidth,
                                     p.m_texHeight,
                                     p.m_format,
                                     convert(p.m_textureType),
                                     p.m_texCount);

      if (m_params.m_useSingleThreadedOGL)
        pool.reset(new TOnDemandSingleThreadedTexturePoolImpl(new TOnDemandSingleThreadedTexturePoolTraits(textureFactory, p.m_texCount)));
      else
        pool.reset(new TOnDemandMultiThreadedTexturePoolImpl(new TOnDemandMultiThreadedTexturePoolTraits(textureFactory, p.m_texCount)));

      pool->SetIsDebugging(p.m_isDebugging);
    }
    else
      LOG(LINFO, ("no ", convert(p.m_textureType), " resource"));
  }

  TTexturePool * ResourceManager::texturePool(ETextureType type)
  {
    return m_texturePools[type].get();
  }

  shared_ptr<gl::BaseTexture> const & ResourceManager::getTexture(string const & name)
  {
    TStaticTextures::const_iterator it = m_staticTextures.find(name);
    ASSERT ( it != m_staticTextures.end(), () );
    return it->second;
  }

  void ResourceManager::loadSkin(shared_ptr<ResourceManager> const & rm,
                                 vector<shared_ptr<ResourceCache> > & caches)
  {
    SkinLoader loader(rm, caches);

    ReaderSource<MemReader> source(MemReader(rm->m_skinBuffer.c_str(), rm->m_skinBuffer.size()));
    if (!ParseXML(source, loader))
      LOG(LERROR, ("Error parsing skin"));
  }

  ResourceManager::Params const & ResourceManager::params() const
  {
    return m_params;
  }

  GlyphCache * ResourceManager::glyphCache(int threadSlot)
  {
    return threadSlot == -1 ? 0 : m_threadSlots[threadSlot].m_glyphCache.get();
  }

  void ResourceManager::addFonts(vector<string> const & fontNames)
  {
    for (unsigned i = 0; i < m_threadSlots.size(); ++i)
      m_threadSlots[i].m_glyphCache->addFonts(fontNames);
  }

  /*
  void ResourceManager::memoryWarning()
  {
  }

  void ResourceManager::enterBackground()
  {
    threads::MutexGuard guard(m_mutex);

    for (unsigned i = 0; i < m_texturePools.size(); ++i)
      if (m_texturePools[i].get())
        m_texturePools[i]->EnterBackground();

    for (unsigned i = 0; i < m_storagePools.size(); ++i)
      if (m_storagePools[i].get())
        m_storagePools[i]->EnterBackground();
  }

  void ResourceManager::enterForeground()
  {
    threads::MutexGuard guard(m_mutex);

    for (unsigned i = 0; i < m_texturePools.size(); ++i)
      if (m_texturePools[i].get())
        m_texturePools[i]->EnterForeground();

    for (unsigned i = 0; i < m_storagePools.size(); ++i)
      if (m_storagePools[i].get())
        m_storagePools[i]->EnterForeground();
  }
  */

  shared_ptr<graphics::gl::BaseTexture> ResourceManager::createRenderTarget(unsigned w, unsigned h)
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

  int ResourceManager::renderThreadSlot(int threadNum) const
  {
    ASSERT(threadNum < m_params.m_renderThreadsCount, (threadNum, m_params.m_renderThreadsCount));
    return 1 + threadNum;
  }

  int ResourceManager::guiThreadSlot() const
  {
    return 0;
  }

  int ResourceManager::cacheThreadSlot() const
  {
    return 1 + m_params.m_renderThreadsCount;
  }

  void ResourceManager::updatePoolState()
  {
    for (unsigned i = 0; i < m_texturePools.size(); ++i)
      if (m_texturePools[i].get())
        m_texturePools[i]->UpdateState();

    for (unsigned i = 0; i < m_storagePools.size(); ++i)
      if (m_storagePools[i].get())
        m_storagePools[i]->UpdateState();
  }

  void ResourceManager::cancel()
  {
    for (unsigned i = 0; i < m_texturePools.size(); ++i)
      if (m_texturePools[i].get())
        m_texturePools[i]->Cancel();

    for (unsigned i = 0; i < m_storagePools.size(); ++i)
      if (m_storagePools[i].get())
        m_storagePools[i]->Cancel();
  }

  bool ResourceManager::useReadPixelsToSynchronize() const
  {
    return m_params.m_useReadPixelsToSynchronize;
  }

  gl::ProgramManager * ResourceManager::programManager(int threadSlot)
  {
    return threadSlot == -1 ? 0 : m_threadSlots[threadSlot].m_programManager.get();
  }
}
