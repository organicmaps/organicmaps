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
    ASSERT(m_indexer == nullptr, ());
  }

  virtual ref_ptr<ResourceInfo> FindResource(Key const & key, bool & newResource)
  {
    ASSERT(m_indexer != nullptr, ());
    if (key.GetType() != TResourceType)
      return ref_ptr<ResourceInfo>();

    return m_indexer->MapResource(static_cast<TResourceKey const &>(key), newResource);
  }

  virtual void UpdateState()
  {
    ASSERT(m_indexer != nullptr, ());
    Bind();
    m_indexer->UploadResources(make_ref<Texture>(this));
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

  void Init(ref_ptr<TIndexer> indexer, TextureParams const & params)
  {
    Init(indexer, params, make_ref<void>(nullptr));
  }

  void Init(ref_ptr<TIndexer> indexer, TextureParams const & params, ref_ptr<void> data)
  {
    m_indexer = indexer;
    Create(params.m_size.x, params.m_size.y, params.m_format, data);
    SetFilterParams(params.m_minFilter, params.m_magFilter);
  }

  void Reset()
  {
    m_indexer = ref_ptr<TIndexer>();
  }

private:
  ref_ptr<TIndexer> m_indexer;
};

}
