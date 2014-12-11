#pragma once

#include "texture.hpp"

namespace dp
{

template<typename TIndexer, typename TResourceKey, Texture::ResourceType TResourceType>
class DynamicTexture : public Texture
{
public:
  DynamicTexture(m2::PointU const & size, dp::TextureFormat format)
    : m_indexer(size)
  {
    Create(size.x, size.y, format);
    SetFilterParams(m_indexer.GetMinFilter(), m_indexer.GetMagFilter());
  }

  virtual ResourceInfo const * FindResource(Key const & key) const
  {
    if (key.GetType() != TResourceType)
      return NULL;

    return m_indexer.MapResource(static_cast<TResourceKey const &>(key));
  }

  virtual void UpdateState()
  {
    Bind();
    m_indexer.UploadResources(MakeStackRefPointer<Texture>(this));
  }

private:
  mutable TIndexer m_indexer;
};

}
