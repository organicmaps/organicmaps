#include "graphics/resource_manager.hpp"

#include "graphics/opengl/opengl.hpp"
#include "graphics/opengl/base_texture.hpp"
#include "graphics/opengl/data_traits.hpp"
#include "graphics/opengl/storage.hpp"
#include "graphics/opengl/texture.hpp"
#include "graphics/opengl/buffer_object.hpp"

#include "graphics/resource_manager.hpp"
#include "graphics/resource_cache.hpp"
#include "graphics/icon.hpp"
#include "graphics/skin_loader.hpp"

#include "coding/file_reader.hpp"
#include "coding/parse_xml.hpp"

#include "base/logging.hpp"
#include "base/exception.hpp"

#include "indexer/map_style_reader.hpp"

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
      LOG(LDEBUG/*LWARNING*/, ("Can't get OpenGL name"));
    }
  }
}

  ResourceManager::Params::Params()
    : m_texFormat(graphics::Data4Bpp),
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
    GetGLStringSafe(GL_VERSION, m_versionName);
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

    /*if (isGPU("Qualcomm", "Adreno", false))
      m_texRtFormat = graphics::Data8Bpp;

    if (isGPU("Samsung Electronics", "FIMG-3DSE", false))
      m_texRtFormat = graphics::Data8Bpp;*/

    if (isGPU("Broadcom", "VideoCore IV HW", false))
      m_texRtFormat = graphics::Data8Bpp;

    /*if (isGPU("Imagination Technologies", "PowerVR MBX", false))
      m_texRtFormat = graphics::Data8Bpp;*/

    m_rgba4RenderBuffer = false;
#ifdef OMIM_OS_ANDROID
    if (isGPU("NVIDIA Corporation", "NVIDIA Tegra", false))
      m_rgba4RenderBuffer = true;
#endif

    LOG(LDEBUG, ("using GL_RGBA4 for color buffer : ", m_rgba4RenderBuffer));
    LOG(LDEBUG, ("selected", graphics::formatName(m_texRtFormat), "format for tile textures"));
  }

  bool ResourceManager::Params::canUseNPOTextures()
  {
    return graphics::gl::HasExtension("GL_OES_texture_npot");
  }

  void ResourceManager::loadSkinInfoAndTexture(string const & skinFileName, EDensity density)
  {
    try
    {
      ReaderPtr<Reader> reader(GetStyleReader().GetResourceReader(skinFileName, convert(density)));
      reader.ReadAsString(m_skinBuffer);

      size_t i = m_skinBuffer.find("file=\"", 0);
      if (i == string::npos)
        MYTHROW(RootException, ("Invalid skin file"));
      i += strlen("file=\"");

      size_t const j = m_skinBuffer.find('\"', i);
      if (j == string::npos)
        MYTHROW(RootException, ("Invalid skin file"));

      string const textureFileName = m_skinBuffer.substr(i, j-i);
      m_staticTextures[textureFileName] = make_shared<TStaticTexture>(textureFileName, density);
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
      LOG(LDEBUG, ("buffer objects are unsupported. using client vertex array instead."));
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
                             p.m_exactDensityDPI,
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
      LOG(LDEBUG, ("initializing", convert(p.m_storageType), "resource pool. vbSize=", p.m_vbSize, ", ibSize=", p.m_ibSize));
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
      LOG(LDEBUG, ("no ", convert(p.m_storageType), " resource"));
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
      LOG(LDEBUG, ("no ", convert(p.m_textureType), " resource"));
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
                                 shared_ptr<ResourceCache> & cache)
  {
    SkinLoader loader([&rm, &cache](m2::RectU const & rect, string const & symbolName,
                      int32_t id, string const & fileName)
    {
      if (cache == nullptr)
        cache.reset(new ResourceCache(rm, fileName, 0));
      Icon * icon = new Icon(rect, 0, Icon::Info(symbolName));
      cache->m_resources[id] = shared_ptr<Resource>(icon);
      cache->m_infos[&icon->m_info] = id;
    });

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

  shared_ptr<graphics::gl::BaseTexture> ResourceManager::createRenderTarget(unsigned w, unsigned h)
  {
    switch (m_params.m_texRtFormat)
    {
    case Data8Bpp:
      return make_shared<gl::Texture<RGBA8Traits, false>>(w, h);
    case Data4Bpp:
      return make_shared<gl::Texture<RGBA4Traits, false>>(w, h);
    case Data565Bpp:
      return make_shared<gl::Texture<RGB565Traits, false>>(w, h);
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

  gl::ProgramManager * ResourceManager::programManager(int threadSlot)
  {
    return threadSlot == -1 ? 0 : m_threadSlots[threadSlot].m_programManager.get();
  }
}
