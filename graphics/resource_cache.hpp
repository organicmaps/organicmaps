#pragma once

#include "std/shared_ptr.hpp"
#include "std/map.hpp"

#include "geometry/packer.hpp"
#include "geometry/rect2d.hpp"

#include "graphics/resource.hpp"
#include "graphics/packets_queue.hpp"
#include "graphics/defines.hpp"
#include "graphics/resource_manager.hpp"

namespace graphics
{
  namespace gl
  {
    class BaseTexture;
  }

  class ResourceManager;

  class ResourceCache
  {
  public:

    typedef m2::Packer::overflowFn handlesOverflowFn;

    typedef vector<shared_ptr<Resource> > TUploadQueue;

  private:

    typedef map<uint32_t, shared_ptr<Resource> > TResources;

    TResources m_resources;
    TResources m_parentResources;

    typedef map<Resource::Info const*, uint32_t, Resource::LessThan> TResourceInfos;
    TResourceInfos m_infos;

    /// made mutable to implement lazy reservation of texture
    /// @{
    mutable shared_ptr<gl::BaseTexture> m_texture;
    mutable shared_ptr<ResourceManager> m_resourceManager;
    /// @}

    m2::Packer m_packer;

    TUploadQueue m_uploadQueue;

    ETextureType m_textureType;
    EStorageType m_storageType;
    uint32_t m_pipelineID;

    /// number of pending rendering commands,
    /// that are using this skin_page
    uint32_t m_activeCommands;

    friend class ResourceManager;

  public:

    void clearHandles();
    void clear();

    bool hasData();
    TUploadQueue const & uploadQueue() const;
    void clearUploadQueue();

    void checkTexture() const;

    uint8_t pipelineID() const;
    void setPipelineID(uint8_t pipelineID);

    /// creation of detached page
    ResourceCache();

    /// creation of a static page
    ResourceCache(shared_ptr<ResourceManager> const & resourceManager,
                  string const & name,
                  uint8_t pipelineID);

    /// creation of a dynamic page
    ResourceCache(shared_ptr<ResourceManager> const & resourceManager,
                  ETextureType type,
                  uint8_t pipelineID);

    void reserveTexture() const;
    void resetTexture();
    void createPacker();

    uint32_t findInfo(Resource::Info const & info) const;
    uint32_t mapInfo(Resource::Info const & info);
    uint32_t addParentInfo(Resource::Info const & fullInfo);
    bool hasRoom(Resource::Info const & info) const;

    Resource * fromID(uint32_t idx) const;

    void setType(ETextureType textureType);
    ETextureType type() const;
    shared_ptr<ResourceManager> const & resourceManager() const;

    void addHandlesOverflowFn(handlesOverflowFn const & fn, int priority);

    bool hasTexture() const;
    shared_ptr<gl::BaseTexture> const & texture() const;
    void setTexture(shared_ptr<gl::BaseTexture> const & t);

    TTexturePool * texturePool() const;
  };
}
