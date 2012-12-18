#include "pipeline_manager.hpp"
#include "resource_cache.hpp"
#include "resource_manager.hpp"

#include "opengl/buffer_object.hpp"

#include "../platform/platform.hpp"

#include "../base/logging.hpp"

#include "../std/iterator.hpp"
#include "../std/bind.hpp"
#include "../std/numeric.hpp"

namespace graphics
{
  PipelinesManager::PipelinesManager(Params const & p)
    : base_t(p)
  {
    vector<shared_ptr<ResourceCache> > caches;
    loadSkin(resourceManager(), p.m_skinName, caches);

    m_staticPagesCount = caches.size();
    m_startStaticPage = reservePipelines(caches,
                                         EMediumStorage);


    m_dynamicPagesCount = 2;
    m_startDynamicPage = reservePipelines(m_dynamicPagesCount,
                                          ELargeTexture,
                                          ELargeStorage);
    m_dynamicPage = m_startDynamicPage;
  }

  void PipelinesManager::GeometryPipeline::checkStorage(shared_ptr<ResourceManager> const & rm) const
  {
    if (!m_hasStorage)
    {
      if (m_storageType != EInvalidStorage)
        m_storage = rm->storagePool(m_storageType)->Reserve();
      else
      {
        LOG(LERROR, ("invalid storage type in checkStorage"));
        return;
      }

      if (m_storage.m_vertices && m_storage.m_indices)
      {
        m_maxVertices = m_storage.m_vertices->size() / sizeof(gl::Vertex);
        m_maxIndices = m_storage.m_indices->size() / sizeof(unsigned short);

        if (!m_storage.m_vertices->isLocked())
          m_storage.m_vertices->lock();
        if (!m_storage.m_indices->isLocked())
          m_storage.m_indices->lock();

        m_vertices = static_cast<gl::Vertex*>(m_storage.m_vertices->data());
        m_indices = static_cast<unsigned short*>(m_storage.m_indices->data());

        m_hasStorage = true;
      }
      else
      {
        m_maxVertices = 0;
        m_maxIndices = 0;

        m_vertices = 0;
        m_indices = 0;

        m_hasStorage = false;
      }
    }
  }

  ETextureType PipelinesManager::GeometryPipeline::textureType() const
  {
    return m_cache->type();
  }

  void PipelinesManager::GeometryPipeline::setTextureType(ETextureType type)
  {
    m_cache->setType(type);
  }

  void PipelinesManager::GeometryPipeline::setStorageType(EStorageType type)
  {
    m_storageType = type;
  }

  unsigned PipelinesManager::reservePipelines(unsigned count,
                                              ETextureType textureType,
                                              EStorageType storageType)
  {
    vector<shared_ptr<ResourceCache> > v;

    for (unsigned i = 0; i < count; ++i)
      v.push_back(make_shared_ptr(new ResourceCache(resourceManager(),
                                                    textureType,
                                                    pipelinesCount() + i)));

    return reservePipelines(v, storageType);
  }

  unsigned PipelinesManager::reservePipelines(vector<shared_ptr<ResourceCache> > const & caches,
                                              EStorageType storageType)
  {
    unsigned res = m_pipelines.size();

    for (unsigned i = 0; i < caches.size(); ++i)
    {
      GeometryPipeline p;

      p.m_cache = caches[i];
      p.m_currentIndex = 0;
      p.m_currentVertex = 0;
      p.m_hasStorage = false;

      p.m_maxVertices = 0;
      p.m_maxIndices = 0;

      p.m_vertices = 0;
      p.m_indices = 0;

      p.m_storageType = storageType;

      m_pipelines.push_back(p);
      m_clearPageFns.push_back(clearPageFns());

      int pipelineID = res + i;

      addClearPageFn(pipelineID, bind(&PipelinesManager::freeTexture, this, pipelineID), 99);
      addClearPageFn(pipelineID, bind(&PipelinesManager::clearPageHandles, this, pipelineID), 0);

      p.m_cache->addHandlesOverflowFn(bind(&PipelinesManager::onDynamicOverflow, this, pipelineID), 0);
    }

    return res;
  }

  PipelinesManager::~PipelinesManager()
  {
    for (size_t i = 0; i < m_pipelines.size(); ++i)
    {
      discardPipeline(i);
      freePipeline(i);
      if (m_pipelines[i].textureType() != EStaticTexture)
        freeTexture(i);
    }
  }

  pair<uint8_t, uint32_t> PipelinesManager::unpackID(uint32_t id) const
  {
    uint8_t pipelineID = (id & 0xFF000000) >> 24;
    uint32_t h = (id & 0x00FFFFFF);
    return make_pair<uint8_t, uint32_t>(pipelineID, h);
  }

  uint32_t PipelinesManager::packID(uint8_t pipelineID, uint32_t handle) const
  {
    uint32_t pipelineIDMask = (uint32_t)pipelineID << 24;
    uint32_t h = (handle & 0x00FFFFFF);
    return (uint32_t)(pipelineIDMask | h);
  }

  Resource const * PipelinesManager::fromID(uint32_t id)
  {
    if (id == invalidHandle())
      return 0;

    id_pair_t p = unpackID(id);

    ASSERT(p.first < m_pipelines.size(), ());
    return m_pipelines[p.first].m_cache->fromID(p.second);
  }

  uint32_t PipelinesManager::mapInfo(Resource::Info const & info)
  {
    uint32_t res = invalidPageHandle();

    for (uint8_t i = 0; i < m_pipelines.size(); ++i)
    {
      res = m_pipelines[i].m_cache->findInfo(info);
      if (res != invalidPageHandle())
        return packID(i, res);
    }

    if (!m_pipelines[m_dynamicPage].m_cache->hasRoom(info))
      flushDynamicPage();

    return packID(m_dynamicPage,
                  m_pipelines[m_dynamicPage].m_cache->mapInfo(info));
  }

  uint32_t PipelinesManager::findInfo(Resource::Info const & info)
  {
    uint32_t res = invalidPageHandle();

    for (uint8_t i = 0; i < m_pipelines.size(); ++i)
    {
      res = m_pipelines[i].m_cache->findInfo(info);
      if (res != invalidPageHandle())
        return packID(i, res);
    }

    return res;
  }

  bool PipelinesManager::mapInfo(Resource::Info const * const * infos,
                                 uint32_t * ids,
                                 size_t count)
  {
    int startDynamicPage = m_dynamicPage;
    int cycles = 0;

    int i = 0;

    do
    {
      ids[i] = m_pipelines[m_dynamicPage].m_cache->findInfo(*infos[i]);

      if ((ids[i] == invalidPageHandle())
       || (unpackID(ids[i]).first != m_dynamicPage))
      {
        /// try to pack on the currentDynamicPage
        while (!m_pipelines[m_dynamicPage].m_cache->hasRoom(*infos[i]))
        {
          /// no room - flush the page
          flushDynamicPage();

          if (startDynamicPage == m_dynamicPage)
            cycles += 1;

          /// there could be maximum 2 cycles to
          /// pack the sequence as a whole.
          /// second cycle is necessary as the first one
          /// could possibly run on partially packed skin pages.
          if (cycles == 2)
            return false;

          /// re-start packing
          i = 0;
        }

        ids[i] = packID(m_dynamicPage,
                        m_pipelines[m_dynamicPage].m_cache->mapInfo(*infos[i]));
      }

      ++i;
    }
    while (i != count);

    return true;
  }

  void PipelinesManager::addClearPageFn(int pipelineID, clearPageFn fn, int priority)
  {
    m_clearPageFns[pipelineID].push(std::pair<size_t, clearPageFn>(priority, fn));
  }

  void PipelinesManager::callClearPageFns(int pipelineID)
  {
    clearPageFns handlersCopy = m_clearPageFns[pipelineID];
    while (!handlersCopy.empty())
    {
      handlersCopy.top().second();
      handlersCopy.pop();
    }
  }

  void PipelinesManager::clearPageHandles(int pipelineID)
  {
    m_pipelines[pipelineID].m_cache->clearHandles();
  }

  /// This function is set to perform as a callback on texture or handles overflow
  /// BUT! Never called on texture overflow, as this situation
  /// is explicitly checked in the mapXXX() functions.
  void PipelinesManager::onDynamicOverflow(int pipelineID)
  {
    LOG(LINFO, ("DynamicPage flushing, pipelineID=", (uint32_t)pipelineID));
    flushDynamicPage();
  }

  bool PipelinesManager::isDynamicPage(int i) const
  {
    return (i >= m_startDynamicPage) && (i < m_startDynamicPage + m_dynamicPagesCount);
  }

  void PipelinesManager::flushDynamicPage()
  {
    callClearPageFns(m_dynamicPage);
    changeDynamicPage();
  }

  int PipelinesManager::nextDynamicPage() const
  {
    if (m_dynamicPage == m_startDynamicPage + m_dynamicPagesCount - 1)
      return m_startDynamicPage;
    else
      return m_dynamicPage + 1;
  }

  void PipelinesManager::changeDynamicPage()
  {
    m_dynamicPage = nextDynamicPage();
  }

  int PipelinesManager::nextPage(int i) const
  {
    ASSERT(i < m_pipelines.size(), ());

    if (isDynamicPage(i))
      return nextDynamicPage();

    /// for static and text pages return same index as passed in.
    return i;
  }

  void PipelinesManager::changePage(int i)
  {
    if (isDynamicPage(i))
      changeDynamicPage();
  }

  uint32_t PipelinesManager::invalidHandle() const
  {
    return 0xFFFFFFFF;
  }

  uint32_t PipelinesManager::invalidPageHandle() const
  {
    return 0x00FFFFFF;
  }

  uint8_t PipelinesManager::dynamicPage() const
  {
    return m_dynamicPage;
  }

  void PipelinesManager::memoryWarning()
  {
  }

  void PipelinesManager::enterBackground()
  {
  }

  void PipelinesManager::enterForeground()
  {
  }

  void PipelinesManager::clearHandles()
  {
    for (unsigned i = 0; i < m_pipelines.size(); ++i)
      m_pipelines[i].m_cache->clear();
  }

  void PipelinesManager::freeTexture(int pipelineID)
  {
    if (!m_pipelines[pipelineID].m_cache->hasTexture())
      return;

    shared_ptr<gl::BaseTexture> texture = m_pipelines[pipelineID].m_cache->texture();
    TTexturePool * texturePool = 0;

    ETextureType type = m_pipelines[pipelineID].m_cache->type();

    if (type != EStaticTexture)
      texturePool = resourceManager()->texturePool(type);
    else
    {
      LOG(LWARNING, ("texture with EStatic can't be freed."));
      return;
    }

    base_t::freeTexture(texture, texturePool);

    m_pipelines[pipelineID].m_cache->resetTexture();
  }

  void PipelinesManager::freePipeline(int pipelineID)
  {
    GeometryPipeline & pipeline = m_pipelines[pipelineID];

    if (pipeline.m_hasStorage)
    {
      TStoragePool * storagePool = 0;
      if (pipeline.m_storageType != EInvalidStorage)
        storagePool = resourceManager()->storagePool(pipeline.m_storageType);
      else
      {
        LOG(LERROR, ("invalid pipeline type in freePipeline"));
        return;
      }

      base_t::freeStorage(pipeline.m_storage, storagePool);

      pipeline.m_hasStorage = false;
      pipeline.m_storage = gl::Storage();
    }
  }

  bool PipelinesManager::flushPipeline(int pipelineID)
  {
    GeometryPipeline & p = m_pipelines[pipelineID];
    if (p.m_currentIndex)
    {
      if (p.m_cache->hasData())
      {
        uploadResources(&p.m_cache->uploadQueue()[0],
                        p.m_cache->uploadQueue().size(),
                        p.m_cache->texture());
        p.m_cache->clearUploadQueue();
      }

      unlockPipeline(pipelineID);

      drawGeometry(p.m_cache->texture(),
                   p.m_storage,
                   p.m_currentIndex,
                   0,
                   ETriangles);

      discardPipeline(pipelineID);

      if (isDebugging())
      {
        p.m_verticesDrawn += p.m_currentVertex;
        p.m_indicesDrawn += p.m_currentIndex;
        //               LOG(LINFO, ("Pipeline #", i - 1, "draws ", pipeline.m_currentIndex / 3, "/", pipeline.m_maxIndices / 3," triangles"));
      }

      freePipeline(pipelineID);

      p.m_maxIndices = 0;
      p.m_maxVertices = 0;
      p.m_vertices = 0;
      p.m_indices = 0;
      p.m_currentIndex = 0;
      p.m_currentVertex = 0;

      return true;
    }

    return false;
  }

  void PipelinesManager::unlockPipeline(int pipelineID)
  {
    GeometryPipeline & pipeline = m_pipelines[pipelineID];
    base_t::unlockStorage(pipeline.m_storage);
  }

  void PipelinesManager::discardPipeline(int pipelineID)
  {
    GeometryPipeline & pipeline = m_pipelines[pipelineID];

    if (pipeline.m_hasStorage)
      base_t::discardStorage(pipeline.m_storage);
  }

  void PipelinesManager::reset(int pipelineID)
  {
    for (size_t i = 0; i < m_pipelines.size(); ++i)
    {
      if ((pipelineID == -1) || ((size_t)pipelineID == i))
      {
        m_pipelines[i].m_currentVertex = 0;
        m_pipelines[i].m_currentIndex = 0;
      }
    }
  }

  void PipelinesManager::beginFrame()
  {
    base_t::beginFrame();
    reset(-1);
    for (size_t i = 0; i < m_pipelines.size(); ++i)
    {
      m_pipelines[i].m_verticesDrawn = 0;
      m_pipelines[i].m_indicesDrawn = 0;
    }
  }

  void PipelinesManager::endFrame()
  {
    /// Syncronization point.
    enableClipRect(false);

    if (isDebugging())
    {
      for (size_t i = 0; i < m_pipelines.size(); ++i)
        if ((m_pipelines[i].m_verticesDrawn != 0) || (m_pipelines[i].m_indicesDrawn != 0))
          LOG(LINFO, ("pipeline #", i, " vertices=", m_pipelines[i].m_verticesDrawn, ", triangles=", m_pipelines[i].m_indicesDrawn / 3));
    }

    /// is the rendering was cancelled, there possibly could
    /// be "ghost" render styles which are present in internal
    /// skin structures, but aren't rendered onto skin texture.
    /// so we are clearing the whole skin, to ensure that they
    /// are gone(slightly heavy, but very simple solution).
    if (isCancelled())
      clearHandles();

    base_t::endFrame();
  }

  unsigned PipelinesManager::pipelinesCount() const
  {
    return m_pipelines.size();
  }

  PipelinesManager::GeometryPipeline & PipelinesManager::pipeline(int i)
  {
    return m_pipelines[i];
  }

  PipelinesManager::GeometryPipeline const & PipelinesManager::pipeline(int i) const
  {
    return m_pipelines[i];
  }
}
