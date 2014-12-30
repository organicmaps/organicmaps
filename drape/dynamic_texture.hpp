#pragma once

#include "texture.hpp"
#include "glconstants.hpp"

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

  virtual RefPointer<ResourceInfo> FindResource(Key const & key) const
  {
    ASSERT(!m_indexer.IsNull(), ());
    if (key.GetType() != TResourceType)
      return RefPointer<ResourceInfo>();

    return m_indexer->MapResource(static_cast<TResourceKey const &>(key));
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
    m_indexer = indexer;
    vector<uint8_t> buf(params.m_size.x * params.m_size.y * 4, 0);
    Create(params.m_size.x, params.m_size.y, params.m_format, MakeStackRefPointer<void>(buf.data()));
    //Create(params.m_size.x, params.m_size.y, params.m_format);
    SetFilterParams(params.m_minFilter, params.m_magFilter);
  }

  void Reset()
  {
    m_indexer = RefPointer<TIndexer>();
  }

private:
  mutable RefPointer<TIndexer> m_indexer;
};

}
