#pragma once

#include "drape/texture.hpp"
#include "drape/glconstants.hpp"

namespace dp
{

template<typename TIndexer, typename TResourceKey, Texture::ResourceType TResourceType>
class DynamicTexture : public Texture
{
public:
  virtual ~DynamicTexture()
  {
    ASSERT(m_indexer.IsNull(), ());
  }

  virtual RefPointer<ResourceInfo> FindResource(Key const & key, bool & newResource)
  {
    ASSERT(!m_indexer.IsNull(), ());
    if (key.GetType() != TResourceType)
      return RefPointer<ResourceInfo>();

    return m_indexer->MapResource(static_cast<TResourceKey const &>(key), newResource);
  }

  virtual void UpdateState()
  {
    ASSERT(!m_indexer.IsNull(), ());
    Bind();
    m_indexer->UploadResources(MakeStackRefPointer<Texture>(this));
  }

protected:
  DynamicTexture() {}

  struct TextureParams
  {
    m2::PointU m_size;
    dp::TextureFormat m_format;
    glConst m_minFilter;
    glConst m_magFilter;
  };

  void Init(RefPointer<TIndexer> indexer, TextureParams const & params)
  {
    Init(indexer, params, MakeStackRefPointer<void>(nullptr));
  }

  void Init(RefPointer<TIndexer> indexer, TextureParams const & params, RefPointer<void> data)
  {
    m_indexer = indexer;
    Create(params.m_size.x, params.m_size.y, params.m_format, data);
    SetFilterParams(params.m_minFilter, params.m_magFilter);
  }

  void Reset()
  {
    m_indexer = RefPointer<TIndexer>();
  }

private:
  RefPointer<TIndexer> m_indexer;
};

}
