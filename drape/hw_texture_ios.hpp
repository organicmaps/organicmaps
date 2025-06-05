#pragma once

#include "drape/gl_constants.hpp"
#include "drape/hw_texture.hpp"

#include "std/target_os.hpp"

#ifndef OMIM_OS_IPHONE
#error Only for iOS
#endif

#import <CoreVideo/CVOpenGLESTexture.h>
#import <CoreVideo/CVOpenGLESTextureCache.h>
#import <CoreVideo/CVPixelBuffer.h>

namespace dp
{
class HWTextureAllocatorApple : public HWTextureAllocator
{
public:
  HWTextureAllocatorApple();
  ~HWTextureAllocatorApple();

  CVPixelBufferRef CVCreatePixelBuffer(uint32_t width, uint32_t height, dp::TextureFormat format);
  void CVDestroyPixelBuffer(CVPixelBufferRef buffer);

  CVOpenGLESTextureRef CVCreateTexture(CVPixelBufferRef buffer, uint32_t width, uint32_t height, glConst layout,
                                       glConst pixelType);
  void CVDestroyTexture(CVOpenGLESTextureRef texture);

  void RiseFlushFlag();

  drape_ptr<HWTexture> CreateTexture(ref_ptr<dp::GraphicsContext> context) override;
  void Flush() override;

private:
  bool m_needFlush;
  CVOpenGLESTextureCacheRef m_textureCache;
};

class HWTextureApple : public HWTexture
{
  using TBase = HWTexture;

public:
  explicit HWTextureApple(ref_ptr<HWTextureAllocatorApple> allocator);
  ~HWTextureApple();

  void Create(ref_ptr<dp::GraphicsContext> context, Params const & params, ref_ptr<void> data) override;
  void UploadData(ref_ptr<dp::GraphicsContext> context, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                  ref_ptr<void> data) override;
  void Bind(ref_ptr<dp::GraphicsContext> context) const override;
  void SetFilter(TextureFilter filter) override;
  bool Validate() const override;

private:
  void Lock();
  void Unlock();

  CVPixelBufferRef m_directBuffer;
  CVOpenGLESTextureRef m_texture;
  ref_ptr<HWTextureAllocatorApple> m_allocator;

  void * m_directPointer;
};
}  // namespace dp
