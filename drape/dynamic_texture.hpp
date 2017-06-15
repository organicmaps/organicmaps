#pragma once

#include "drape/texture.hpp"
#include "drape/glconstants.hpp"

#include <vector>

namespace dp
{
template<typename TIndexer, typename TResourceKey, Texture::ResourceType TResourceType>
class DynamicTexture : public Texture
{
public:
  ~DynamicTexture() override
  {
    ASSERT(m_indexer == nullptr, ());
  }

  ref_ptr<ResourceInfo> FindResource(Key const & key, bool & newResource) override
  {
    ASSERT(m_indexer != nullptr, ());
    if (key.GetType() != TResourceType)
      return nullptr;

    return m_indexer->MapResource(static_cast<TResourceKey const &>(key), newResource);
  }

  void UpdateState() override
  {
    // Create texture before first uploading.
    if (m_hwTexture == nullptr)
    {
      std::vector<uint8_t> initData(m_params.m_width * m_params.m_height *
                                    GetBytesPerPixel(m_params.m_format), 0);
      Create(m_params, initData.data());
    }

    ASSERT(m_indexer != nullptr, ());
    Bind();
    m_indexer->UploadResources(make_ref(this));
  }

  TextureFormat GetFormat() const override
  {
    return m_hwTexture == nullptr ? m_params.m_format : Texture::GetFormat();
  }

  uint32_t GetWidth() const override
  {
    return m_hwTexture == nullptr ? m_params.m_width : Texture::GetWidth();
  }

  uint32_t GetHeight() const override
  {
    return m_hwTexture == nullptr ? m_params.m_height : Texture::GetHeight();
  }

protected:
  DynamicTexture() {}

  struct TextureParams
  {
    m2::PointU m_size;
    dp::TextureFormat m_format;
    glConst m_filter;
    bool m_usePixelBuffer;
  };

  void Init(ref_ptr<HWTextureAllocator> allocator, ref_ptr<TIndexer> indexer,
            TextureParams const & params)
  {
    m_indexer = indexer;
    m_params.m_allocator = allocator;
    m_params.m_width = params.m_size.x;
    m_params.m_height = params.m_size.y;
    m_params.m_format = params.m_format;
    m_params.m_filter = params.m_filter;
    m_params.m_usePixelBuffer = params.m_usePixelBuffer;
  }

  void Reset()
  {
    m_indexer = nullptr;
  }

  ref_ptr<TIndexer> m_indexer;
  Texture::Params m_params;
};
}  // namespace dp
