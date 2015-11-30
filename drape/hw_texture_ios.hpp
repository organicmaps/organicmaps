#pragma once

#import "hw_texture.hpp"

#ifndef OMIM_OS_IPHONE
  #error Only for ios
#endif

#import <CoreVideo/CVPixelBuffer.h>
#import <CoreVideo/CVOpenGLESTexture.h>
#import <CoreVideo/CVOpenGLESTextureCache.h>

namespace dp
{

class HWTextureAllocatorApple : public HWTextureAllocator
{
public:
  HWTextureAllocatorApple();
  ~HWTextureAllocatorApple();

  CVPixelBufferRef CVCreatePixelBuffer(uint32_t width, uint32_t height, int format);
  void CVDestroyPixelBuffer(CVPixelBufferRef buffer);

  CVOpenGLESTextureRef CVCreateTexture(CVPixelBufferRef buffer, uint32_t width, uint32_t height,
                                   glConst layout, glConst pixelType);
  void CVDestroyTexture(CVOpenGLESTextureRef texture);

  void RiseFlushFlag();

  drape_ptr<HWTexture> CreateTexture() override;
  void Flush() override;

private:
  bool m_needFlush;
  CVOpenGLESTextureCacheRef m_textureCache;
};

class HWTextureApple : public HWTexture
{
  using TBase = HWTexture;

public:
  HWTextureApple(ref_ptr<HWTextureAllocatorApple> allocator);
  ~HWTextureApple();

  void Create(Params const & params, ref_ptr<void> data);
  void UploadData(uint32_t x, uint32_t y, uint32_t width, uint32_t height, ref_ptr<void> data);

private:
  void Lock();
  void Unlock();

private:
  CVPixelBufferRef m_directBuffer;
  CVOpenGLESTextureRef m_texture;
  ref_ptr<HWTextureAllocatorApple> m_allocator;

  void * m_directPointer;
};

}
