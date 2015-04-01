#include "geometry_pipeline.hpp"
#include "resource_manager.hpp"
#include "resource_cache.hpp"
#include "opengl/buffer_object.hpp"

namespace graphics
{
  GeometryPipeline::GeometryPipeline(shared_ptr<ResourceCache> const & cache,
                                     EStorageType storageType,
                                     shared_ptr<ResourceManager> const & rm,
                                     VertexDecl const * decl)
    : m_decl(decl),
      m_cache(cache),
      m_currentVx(0),
      m_currentIdx(0),
      m_maxVx(0),
      m_maxIdx(0),
      m_rm(rm),
      m_storageType(storageType)
  {
  }

  GeometryPipeline::GeometryPipeline(ETextureType textureType,
                                     EStorageType storageType,
                                     shared_ptr<ResourceManager> const & rm,
                                     VertexDecl const * decl,
                                     uint8_t pipelineID)
    : m_decl(decl),
      m_cache(new ResourceCache(rm, textureType, pipelineID)),
      m_currentVx(0),
      m_currentIdx(0),
      m_maxVx(0),
      m_maxIdx(0),
      m_rm(rm),
      m_storageType(storageType)
  {}

  bool GeometryPipeline::hasStorage() const
  {
    return m_storage.isValid();
  }

  bool GeometryPipeline::hasTexture() const
  {
    return m_cache->texture() != 0;
  }

  void GeometryPipeline::checkStorage() const
  {
    if (!m_storage.isValid())
    {
      resetStorage();
      m_storage = m_rm->storagePool(m_storageType)->Reserve();

      if (m_storage.isValid())
      {
        if (!m_storage.m_vertices->isLocked())
          m_storage.m_vertices->lock();
        if (!m_storage.m_indices->isLocked())
          m_storage.m_indices->lock();

        m_decl->initStream(&m_vertexStream,
                           (unsigned char*)m_storage.m_vertices->data());

        m_maxVx = m_storage.m_vertices->size() / m_decl->elemSize();
        m_maxIdx = m_storage.m_indices->size() / sizeof(unsigned short);
      }
    }
  }

  void GeometryPipeline::checkTexture() const
  {
    m_cache->checkTexture();
  }

  void GeometryPipeline::resetTexture()
  {
    m_cache->resetTexture();
  }

  void GeometryPipeline::clearStorage() const
  {
    m_currentVx = 0;
    m_currentIdx = 0;
  }

  void GeometryPipeline::resetStorage() const
  {
    m_storage = gl::Storage();
    m_maxIdx = 0;
    m_maxVx = 0;
    clearStorage();
  }

  bool GeometryPipeline::hasRoom(unsigned verticesCount, unsigned indicesCount) const
  {
    checkStorage();
    if (!hasStorage())
      return false;

    return ((m_currentVx + verticesCount <= m_maxVx)
         && (m_currentIdx + indicesCount <= m_maxIdx));
  }

  void GeometryPipeline::setVertexDecl(VertexDecl * decl)
  {
    m_decl = decl;
  }

  VertexDecl const * GeometryPipeline::vertexDecl() const
  {
    return m_decl;
  }

  VertexStream * GeometryPipeline::vertexStream()
  {
    return &m_vertexStream;
  }

  shared_ptr<gl::Program> const & GeometryPipeline::program() const
  {
    return m_program;
  }

  void GeometryPipeline::setProgram(shared_ptr<gl::Program> const & prg)
  {
    m_program = prg;
  }

  uint8_t GeometryPipeline::pipelineID() const
  {
    return m_cache->pipelineID();
  }

  shared_ptr<gl::BaseTexture> const & GeometryPipeline::texture() const
  {
    return m_cache->texture();
  }

  gl::Storage const & GeometryPipeline::storage() const
  {
    return m_storage;
  }

  TTexturePool * GeometryPipeline::texturePool() const
  {
    return m_cache->texturePool();
  }

  TStoragePool * GeometryPipeline::storagePool() const
  {
    if (m_storageType != EInvalidStorage)
      return m_rm->storagePool(m_storageType);
    else
    {
      LOG(LERROR, ("no storagePool for such storageType", m_storageType));
      return 0;
    }
  }

  bool GeometryPipeline::hasGeometry() const
  {
    return m_storage.isValid() && (m_currentIdx != 0);
  }

  ResourceCache::TUploadQueue const & GeometryPipeline::uploadQueue() const
  {
    return m_cache->uploadQueue();
  }

  void GeometryPipeline::clearUploadQueue()
  {
    return m_cache->clearUploadQueue();
  }

  bool GeometryPipeline::hasUploadData() const
  {
    return m_cache->hasData();
  }

  unsigned GeometryPipeline::vxLeft() const
  {
    checkStorage();

    if (!m_storage.isValid())
      return (unsigned)-1;

    return m_maxVx - m_currentVx;
  }

  unsigned GeometryPipeline::currentVx() const
  {
    return m_currentVx;
  }

  void GeometryPipeline::advanceVx(unsigned elemCnt)
  {
    m_currentVx += elemCnt;
    m_vertexStream.advanceVertex(elemCnt);
  }

  void * GeometryPipeline::vxData()
  {
    return m_storage.m_vertices->data();
  }

  unsigned GeometryPipeline::idxLeft() const
  {
    checkStorage();

    if (!m_storage.isValid())
      return (unsigned)-1;

    return m_maxIdx - m_currentIdx;
  }

  unsigned GeometryPipeline::currentIdx() const
  {
    return m_currentIdx;
  }

  void GeometryPipeline::advanceIdx(unsigned elemCnt)
  {
    m_currentIdx += elemCnt;
  }

  void * GeometryPipeline::idxData()
  {
    return m_storage.m_indices->data();
  }

  void GeometryPipeline::addHandlesOverflowFn(ResourceCache::handlesOverflowFn const & fn,
                                              int priority)
  {
    m_cache->addHandlesOverflowFn(fn, priority);
  }

  shared_ptr<ResourceCache> const & GeometryPipeline::cache() const
  {
    return m_cache;
  }
}
