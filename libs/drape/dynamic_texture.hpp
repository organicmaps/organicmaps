#pragma once

#include "drape/texture.hpp"

#include <atomic>
#include <vector>

namespace dp
{
template <typename TIndexer, typename TResourceKey, Texture::ResourceType TResourceType>
class DynamicTexture : public Texture
{
  using Base = Texture;

public:
  ~DynamicTexture() override { ASSERT(m_indexer == nullptr, ()); }

  ref_ptr<ResourceInfo> FindResource(Key const & key, bool & newResource) override
  {
    ASSERT(m_indexer != nullptr, ());
    if (key.GetType() != TResourceType)
      return nullptr;

    return m_indexer->MapResource(static_cast<TResourceKey const &>(key), newResource);
  }

  void Create(ref_ptr<GraphicsContext> context, Params const & params) override
  {
    ASSERT(Base::IsPowerOfTwo(params.m_width, params.m_height), (params.m_width, params.m_height));
    Base::Create(context, params);
  }

  void Create(ref_ptr<GraphicsContext> context, Params const & params, ref_ptr<void> data) override
  {
    ASSERT(Base::IsPowerOfTwo(params.m_width, params.m_height), (params.m_width, params.m_height));
    Base::Create(context, params, data);
  }

  void UpdateState(ref_ptr<GraphicsContext> context) override
  {
    // Create texture before first uploading.
    if (!m_isInitialized)
    {
      std::vector<uint8_t> initData(m_params.m_width * m_params.m_height * GetBytesPerPixel(m_params.m_format), 0);
      Create(context, m_params, initData.data());
      m_isInitialized = true;
    }

    ASSERT(m_indexer != nullptr, ());
    Bind(context);
    m_indexer->UploadResources(context, make_ref(this));
  }

  TextureFormat GetFormat() const override { return m_params.m_format; }

  uint32_t GetWidth() const override { return m_params.m_width; }

  uint32_t GetHeight() const override { return m_params.m_height; }

  float GetS(uint32_t x) const override { return static_cast<float>(x) / m_params.m_width; }

  float GetT(uint32_t y) const override { return static_cast<float>(y) / m_params.m_height; }

  uint32_t GetID() const override { return m_isInitialized ? Texture::GetID() : 0; }

  void Bind(ref_ptr<dp::GraphicsContext> context) const override
  {
    if (m_isInitialized)
      Texture::Bind(context);
  }

  void SetFilter(TextureFilter filter) override
  {
    if (m_isInitialized)
      Texture::SetFilter(filter);
  }

protected:
  DynamicTexture() : m_isInitialized(false) {}

  struct DynamicTextureParams
  {
    m2::PointU m_size;
    dp::TextureFormat m_format = dp::TextureFormat::Unspecified;
    TextureFilter m_filter = dp::TextureFilter::Nearest;
    bool m_usePixelBuffer = false;
  };

  void Init(ref_ptr<HWTextureAllocator> allocator, ref_ptr<TIndexer> indexer, DynamicTextureParams const & params)
  {
    m_indexer = indexer;
    m_params.m_allocator = allocator;
    m_params.m_width = params.m_size.x;
    m_params.m_height = params.m_size.y;
    m_params.m_format = params.m_format;
    m_params.m_filter = params.m_filter;
    m_params.m_usePixelBuffer = params.m_usePixelBuffer;
    m_params.m_isMutable = true;
  }

  void Reset() { m_indexer = nullptr; }

  ref_ptr<TIndexer> m_indexer;
  Texture::Params m_params;
  std::atomic<bool> m_isInitialized;
};
}  // namespace dp
