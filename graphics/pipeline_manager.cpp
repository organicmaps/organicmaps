#include "pipeline_manager.hpp"
#include "resource_cache.hpp"
#include "resource_manager.hpp"

#include "opengl/buffer_object.hpp"

#include "../platform/platform.hpp"

#include "../base/logging.hpp"

#include "../std/iterator.hpp"
#include "../std/bind.hpp"
#include "../std/numeric.hpp"
#include "../std/utility.hpp"

namespace graphics
{
  PipelinesManager::PipelinesManager(base_t::Params const & p)
    : base_t(p)
  {}

  unsigned PipelinesManager::reservePipelines(unsigned count,
                                              ETextureType textureType,
                                              EStorageType storageType,
                                              VertexDecl const * decl)
  {
    vector<shared_ptr<ResourceCache> > v;

    for (unsigned i = 0; i < count; ++i)
      v.push_back(make_shared<ResourceCache>(resourceManager(),
                                             textureType,
                                             pipelinesCount() + i));

    return reservePipelines(v, storageType, decl);
  }

  unsigned PipelinesManager::reservePipelines(vector<shared_ptr<ResourceCache> > const & caches,
                                              EStorageType storageType,
                                              VertexDecl const * decl)
  {
    unsigned res = m_pipelines.size();

    for (unsigned i = 0; i < caches.size(); ++i)
    {
      GeometryPipeline p(caches[i],
                         storageType,
                         resourceManager(),
                         decl);

      m_pipelines.push_back(p);
      m_clearPageFns.push_back(clearPageFns());

      int pipelineID = res + i;

      addClearPageFn(pipelineID, bind(&PipelinesManager::freeTexture, this, pipelineID), 99);
      addClearPageFn(pipelineID, bind(&PipelinesManager::clearPageHandles, this, pipelineID), 0);

    }

    return res;
  }

  PipelinesManager::~PipelinesManager()
  {
    for (size_t i = 0; i < m_pipelines.size(); ++i)
    {
      discardPipeline(i);
      freePipeline(i);
      freeTexture(i);
    }
  }

  pair<uint8_t, uint32_t> PipelinesManager::unpackID(uint32_t id) const
  {
    uint8_t const pipelineID = (id & 0xFF000000) >> 24;
    uint32_t const h = (id & 0x00FFFFFF);
    return make_pair(pipelineID, h);
  }

  uint32_t PipelinesManager::packID(uint8_t pipelineID, uint32_t handle) const
  {
    uint32_t const pipelineIDMask = (uint32_t)pipelineID << 24;
    uint32_t const h = (handle & 0x00FFFFFF);
    return static_cast<uint32_t>(pipelineIDMask | h);
  }

  Resource const * PipelinesManager::fromID(uint32_t id)
  {
    if (id == invalidHandle())
      return 0;

    id_pair_t p = unpackID(id);

    ASSERT(p.first < m_pipelines.size(), ());
    return m_pipelines[p.first].cache()->fromID(p.second);
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
    m_pipelines[pipelineID].cache()->clearHandles();
  }

  uint32_t PipelinesManager::invalidHandle() const
  {
    return 0xFFFFFFFF;
  }

  uint32_t PipelinesManager::invalidPageHandle() const
  {
    return 0x00FFFFFF;
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
      m_pipelines[i].cache()->clear();
  }

  void PipelinesManager::freeTexture(int pipelineID)
  {
    GeometryPipeline & p = m_pipelines[pipelineID];

    if (p.hasTexture())
    {
      shared_ptr<gl::BaseTexture> texture = p.texture();
      TTexturePool * texturePool = p.texturePool();

      if (texturePool == 0)
        return;

      base_t::freeTexture(texture, texturePool);

      p.resetTexture();
    }
  }

  void PipelinesManager::freePipeline(int pipelineID)
  {
    GeometryPipeline & p = m_pipelines[pipelineID];

    if (p.hasStorage())
    {
      TStoragePool * storagePool = p.storagePool();

      if (storagePool == 0)
        return;

      base_t::freeStorage(p.storage(), storagePool);

      p.resetStorage();
    }
  }

  bool PipelinesManager::flushPipeline(int pipelineID)
  {
    GeometryPipeline & p = m_pipelines[pipelineID];

    if (p.hasGeometry())
    {
      if (p.hasUploadData())
      {
        ResourceCache::TUploadQueue const & uploadQueue = p.uploadQueue();

        uploadResources(&uploadQueue[0],
                        uploadQueue.size(),
                        p.texture());

        p.clearUploadQueue();
      }

      unlockPipeline(pipelineID);

      drawGeometry(p.texture(),
                   p.storage(),
                   p.currentIdx(),
                   0,
                   ETriangles);

      discardPipeline(pipelineID);

      freePipeline(pipelineID);

      resetPipeline(pipelineID);

      return true;
    }

    return false;
  }

  void PipelinesManager::unlockPipeline(int pipelineID)
  {
    GeometryPipeline & pipeline = m_pipelines[pipelineID];
    if (pipeline.hasStorage())
      base_t::unlockStorage(pipeline.storage());
  }

  void PipelinesManager::discardPipeline(int pipelineID)
  {
    GeometryPipeline & pipeline = m_pipelines[pipelineID];
    if (pipeline.hasStorage())
      base_t::discardStorage(pipeline.storage());
  }

  void PipelinesManager::resetPipeline(int pipelineID)
  {
    for (size_t i = 0; i < m_pipelines.size(); ++i)
    {
      if ((pipelineID == -1) || ((size_t)pipelineID == i))
        pipeline(i).resetStorage();
    }
  }

  void PipelinesManager::clearPipeline(int pipelineID)
  {
    for (size_t i = 0; i < m_pipelines.size(); ++i)
    {
      if ((pipelineID == -1) || ((size_t)pipelineID == i))
        pipeline(i).clearStorage();
    }
  }

  void PipelinesManager::beginFrame()
  {
    base_t::beginFrame();
    clearPipeline(-1);
  }

  void PipelinesManager::endFrame()
  {
    enableClipRect(false);

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

  GeometryPipeline & PipelinesManager::pipeline(int i)
  {
    return m_pipelines[i];
  }

  GeometryPipeline const & PipelinesManager::pipeline(int i) const
  {
    return m_pipelines[i];
  }
}
