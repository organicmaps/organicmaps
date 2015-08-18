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
      return nullptr;

    return m_indexer->MapResource(static_cast<TResourceKey const &>(key), newResource);
  }

  virtual void UpdateState()
  {
    ASSERT(m_indexer != nullptr, ());
    Bind();
    m_indexer->UploadResources(make_ref(this));
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

  void Init(ref_ptr<HWTextureAllocator> allocator, ref_ptr<TIndexer> indexer, TextureParams const & params)
  {
    Init(allocator, indexer, params, nullptr);
  }

  void Init(ref_ptr<HWTextureAllocator> allocator, ref_ptr<TIndexer> indexer, TextureParams const & params,
            ref_ptr<void> data)
  {
    m_indexer = indexer;
    Texture::Params p;
    p.m_allocator = allocator;
    p.m_width = params.m_size.x;
    p.m_height = params.m_size.y;
    p.m_format = params.m_format;
    p.m_magFilter = params.m_magFilter;
    p.m_minFilter = params.m_minFilter;

    Create(p, data);
  }

  void Reset()
  {
    m_indexer = nullptr;
  }

  ref_ptr<TIndexer> m_indexer;
};

}
