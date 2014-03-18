#include "resource_cache.hpp"

#include "opengl/texture.hpp"
#include "opengl/data_traits.hpp"

#include "resource_manager.hpp"

#include "../base/logging.hpp"

#include "../std/bind.hpp"
#include "../std/numeric.hpp"

namespace graphics
{
  typedef gl::Texture<DATA_TRAITS, true> TDynamicTexture;

  ResourceCache::ResourceCache()
    : m_textureType(EStaticTexture),
      m_pipelineID(0)
  {}

  ResourceCache::ResourceCache(shared_ptr<ResourceManager> const & resourceManager,
                               string const & name,
                               uint8_t pipelineID)
                             : m_texture(resourceManager->getTexture(name)),
                               m_packer(m_texture->width(), m_texture->height(), 0x00FFFFFF - 1),
                               m_textureType(EStaticTexture),
                               m_pipelineID(pipelineID)
  {
  }

  ResourceCache::ResourceCache(shared_ptr<ResourceManager> const & resourceManager,
                               ETextureType type,
                               uint8_t pipelineID)
    : m_resourceManager(resourceManager),
      m_textureType(type),
      m_pipelineID(pipelineID)
  {
    createPacker();
    /// clear handles will be called only upon handles overflow,
    /// as the texture overflow is processed separately
    addHandlesOverflowFn(bind(&ResourceCache::clearHandles, this), 0);
  }

  void ResourceCache::clearHandles()
  {
    if (m_textureType != graphics::EStaticTexture)
    {
      /// clearing only non-static caches.
      for (TResourceInfos::const_iterator it = m_infos.begin();
         it != m_infos.end();
         ++it)
      m_resources.erase(it->second);

      m_infos.clear();
      m_packer.reset();
    }
  }

  void ResourceCache::clearUploadQueue()
  {
    m_uploadQueue.clear();
  }

  void ResourceCache::clear()
  {
    if (m_textureType != EStaticTexture)
    {
      clearHandles();
      clearUploadQueue();
    }
  }

  uint32_t ResourceCache::findInfo(Resource::Info const & info) const
  {
    TResourceInfos::const_iterator it = m_infos.find(&info);
    if (it == m_infos.end())
      return m_packer.invalidHandle();
    else
      return it->second;
  }

  uint32_t ResourceCache::mapInfo(Resource::Info const & info)
  {
    uint32_t foundHandle = findInfo(info);
    if (foundHandle != m_packer.invalidHandle())
      return foundHandle;

    m2::PointU sz = info.resourceSize();

    m2::Packer::handle_t h = m_packer.pack(sz.x, sz.y);

    m2::RectU texRect = m_packer.find(h).second;
    shared_ptr<Resource> resource(info.createResource(texRect, m_pipelineID));

    m_resources[h] = resource;
    m_infos[resource->info()] = h;
    m_uploadQueue.push_back(resource);

    return h;
  }

  uint32_t ResourceCache::addParentInfo(Resource::Info const & fullInfo)
  {
    uint32_t id = findInfo(fullInfo.cacheKey());
    Resource * r = fromID(id);

    shared_ptr<Resource> resource(fullInfo.createResource(r->m_texRect, r->m_pipelineID));

    uint32_t newID = m_packer.freeHandle();

    m_parentResources[newID] = resource;
    m_infos[resource->info()] = newID;

    return newID;
  }

  bool ResourceCache::hasRoom(Resource::Info const & info) const
  {
    m2::PointU sz = info.resourceSize();
    return m_packer.hasRoom(sz.x, sz.y);
  }

  void ResourceCache::setType(ETextureType textureType)
  {
    m_textureType = textureType;
    createPacker();
    if (m_textureType != EStaticTexture)
      addHandlesOverflowFn(bind(&ResourceCache::clearHandles, this), 0);
  }

  ETextureType ResourceCache::type() const
  {
    return m_textureType;
  }

  bool ResourceCache::hasData()
  {
    return !m_uploadQueue.empty();
  }

  ResourceCache::TUploadQueue const & ResourceCache::uploadQueue() const
  {
    return m_uploadQueue;
  }

  void ResourceCache::checkTexture() const
  {
    if ((m_textureType != EStaticTexture) && (m_texture == 0))
      reserveTexture();
  }

  void ResourceCache::setPipelineID(uint8_t pipelineID)
  {
    m_pipelineID = pipelineID;
    for (TResources::iterator it = m_resources.begin();
         it != m_resources.end();
         ++it)
      it->second->m_pipelineID = pipelineID;
  }

  uint8_t ResourceCache::pipelineID() const
  {
    return m_pipelineID;
  }

  Resource * ResourceCache::fromID(uint32_t idx) const
  {
    TResources::const_iterator it = m_parentResources.find(idx);

    it = m_parentResources.find(idx);

    if (it != m_parentResources.end())
      return it->second.get();

    it = m_resources.find(idx);

    if (it != m_resources.end())
      return it->second.get();

    return 0;
  }

  shared_ptr<gl::BaseTexture> const & ResourceCache::texture() const
  {
    checkTexture();
    return m_texture;
  }

  bool ResourceCache::hasTexture() const
  {
    return m_texture != 0;
  }

  void ResourceCache::setTexture(shared_ptr<gl::BaseTexture> const & texture)
  {
    m_texture = texture;
    m_packer = m2::Packer(texture->width(),
                          texture->height(),
                          0x00FFFFFF - 1);
  }

  void ResourceCache::resetTexture()
  {
    m_texture.reset();
  }

  void ResourceCache::reserveTexture() const
  {
    if (m_textureType != EStaticTexture)
      m_texture = m_resourceManager->texturePool(m_textureType)->Reserve();
    else
      LOG(LDEBUG, ("reserveTexture call for with invalid type param"));
  }

  TTexturePool * ResourceCache::texturePool() const
  {
    if (m_textureType == EStaticTexture)
      return 0;

    if (m_textureType != EInvalidTexture)
      return m_resourceManager->texturePool(m_textureType);
    else
    {
      LOG(LDEBUG/*LWARNING*/, ("no texturePool with such type", m_textureType));
      return 0;
    }
  }

  void ResourceCache::createPacker()
  {
    if (m_textureType != EStaticTexture)
      m_packer = m2::Packer(m_resourceManager->params().m_textureParams[m_textureType].m_texWidth,
                            m_resourceManager->params().m_textureParams[m_textureType].m_texHeight,
                            0x00FFFFFF - 1);
    else
      LOG(LDEBUG, ("createPacker call for invalid type param"));
  }

  shared_ptr<ResourceManager> const & ResourceCache::resourceManager() const
  {
    return m_resourceManager;
  }

  void ResourceCache::addHandlesOverflowFn(handlesOverflowFn const & fn, int priority)
  {
    m_packer.addOverflowFn(fn, priority);
  }
}
