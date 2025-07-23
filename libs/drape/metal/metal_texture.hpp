#pragma once
#import <MetalKit/MetalKit.h>

#include "drape/hw_texture.hpp"
#include "drape/pointers.hpp"

namespace dp
{
namespace metal
{
class MetalTextureAllocator : public HWTextureAllocator
{
public:
  drape_ptr<HWTexture> CreateTexture(ref_ptr<dp::GraphicsContext> context) override;
  void Flush() override {}
};

class MetalTexture : public HWTexture
{
  using Base = HWTexture;

public:
  explicit MetalTexture(ref_ptr<MetalTextureAllocator>) {}

  void Create(ref_ptr<dp::GraphicsContext> context, Params const & params, ref_ptr<void> data) override;
  void UploadData(ref_ptr<dp::GraphicsContext> context, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                  ref_ptr<void> data) override;
  void Bind(ref_ptr<dp::GraphicsContext> context) const override {}
  void SetFilter(TextureFilter filter) override;
  bool Validate() const override;

  id<MTLTexture> GetTexture() const { return m_texture; }

private:
  id<MTLTexture> m_texture;
  bool m_isMutable = false;
};
}  // namespace metal
}  // namespace dp
