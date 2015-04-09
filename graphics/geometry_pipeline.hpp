#pragma once

#include "std/shared_ptr.hpp"

#include "graphics/opengl/storage.hpp"
#include "graphics/opengl/program.hpp"

#include "graphics/defines.hpp"
#include "graphics/vertex_decl.hpp"
#include "graphics/resource_cache.hpp"
#include "graphics/resource_manager.hpp"

namespace graphics
{
  class ResourceCache;
  class ResourceManager;

  /// Single pipeline for batchable geometry.
  class GeometryPipeline
  {
  private:

    /// VertexDeclaration, which sets how the
    /// vertex is interpreted into m_storage.m_vertexBuffer
    VertexDecl const * m_decl;

    /// Stream of interpreted data from m_storage,
    /// created according to m_decl.
    mutable VertexStream m_vertexStream;

    /// Program, which is used for this pipeline.
    /// Different pipelines could use different programs.
    shared_ptr<gl::Program> m_program;

    /// ResourceCache, which collects Resource's which are used
    /// by objects while batching them into this pipeline
    shared_ptr<ResourceCache> m_cache;
    /// Storage, which contains vertex and index data
    mutable gl::Storage m_storage;

    /// current rendering position
    /// @{
    mutable size_t m_currentVx;
    mutable size_t m_currentIdx;

    mutable size_t m_maxVx;
    mutable size_t m_maxIdx;
    /// @}

    /// resource manager to interact with pool's
    shared_ptr<ResourceManager> m_rm;

    /// type of storage pools
    EStorageType m_storageType;

  public:

    typedef ResourceCache::handlesOverflowFn handlesOverflowFn;

    /// Constructor of static GeometryPipeline from predefined
    /// ResourceCache(possibly loaded from file)
    GeometryPipeline(shared_ptr<ResourceCache> const & cache,
                     EStorageType storageType,
                     shared_ptr<ResourceManager> const & rm,
                     VertexDecl const * decl);

    /// Constructor of dynamic GeometryPipeline from texture
    /// and storage pool types and resourceManager
    GeometryPipeline(ETextureType textureType,
                     EStorageType storageType,
                     shared_ptr<ResourceManager> const & rm,
                     VertexDecl const * decl,
                     uint8_t pipelineID);

    /// Setting VertexDeclaration on this pipeline, which will define
    /// how to interpret data into m_storage.m_vertexBuffer
    void setVertexDecl(VertexDecl * decl);
    VertexDecl const * vertexDecl() const;

    /// Get structured stream of vertex data from m_storage.
    VertexStream * vertexStream();

    /// Does this pipeline holds enough free room for the
    /// specified amount of vertices and indices.
    bool hasRoom(unsigned verticesCount,
                 unsigned indicesCount) const;

    /// Get ResourceCache, associated with this pipeline
    shared_ptr<ResourceCache> const & cache() const;

    void setProgram(shared_ptr<gl::Program> const & prg);
    shared_ptr<gl::Program> const & program() const;

    /// ID of this pipeline
    uint8_t pipelineID() const;

    /// Working with texture and ResourceCache
    /// @{

    /// Checking, whether this pipeline
    /// has texture associated with it.
    bool hasTexture() const;
    /// Reserve texture if there is no texture at this pipeline.
    void checkTexture() const;
    /// Get the texture associated with this pipeline.
    shared_ptr<gl::BaseTexture> const & texture() const;
    /// Get the texture pool this pipeline get it's texture from.
    TTexturePool * texturePool() const;
    /// Reset the texture, associated with this pipeline.
    void resetTexture();
    /// Does this pipeline has something to upload before render?
    bool hasUploadData() const;
    /// Get the queue of resources, which are waiting for upload
    ResourceCache::TUploadQueue const & uploadQueue() const;
    /// Clear upload queue
    void clearUploadQueue();

    /// @}

    /// Working with Storage
    /// @{

    /// Checking, whether we should allocate the storage for this pipeline.
    void checkStorage() const;
    /// Checking, whether we have a valid storage
    /// associated with this pipeline.
    bool hasStorage() const;
    /// getting storage associated with this pipeline
    gl::Storage const & storage() const;
    /// Get the storage pool this pipeline get it's storages from
    TStoragePool * storagePool() const;
    /// Reset storage and rendering info,
    /// associated with this pipeline.
    void resetStorage() const;
    /// Clear rendered geometry information.
    void clearStorage() const;
    /// Does this pipeline has something to render?
    bool hasGeometry() const;

    /// How much more vertices with the current VertexDecl
    /// could be fitted into this pipeline
    unsigned vxLeft() const;
    /// Current vertex number
    unsigned currentVx() const;
    /// Advance current vertex number
    void advanceVx(unsigned elemCnt);
    /// Get direct pointer to vertex data
    void * vxData();
    /// Get VertexDecl
    VertexDecl * decl() const;

    /// How much more indices could be fitted into this pipeline
    unsigned idxLeft() const;
    /// Current index number
    unsigned currentIdx() const;
    /// Advance current index number
    void advanceIdx(unsigned elemCnt);
    /// Get direct pointer to index data
    void * idxData();

    /// @}

    /// Add function, which will be called upon handles overflow.
    void addHandlesOverflowFn(ResourceCache::handlesOverflowFn const & fn,
                              int priority);
  };
}
