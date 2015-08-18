#include "hw_texture_ios.hpp"

#include "base/logging.hpp"

#include "drape/glfunctions.hpp"

#import <QuartzCore/CAEAGLLayer.h>

#import <Foundation/NSDictionary.h>
#import <Foundation/NSValue.h>

#include <boost/gil/algorithm.hpp>
#include <boost/gil/typedefs.hpp>

using boost::gil::gray8c_pixel_t;
using boost::gil::gray8_pixel_t;
using boost::gil::gray8c_view_t;
using boost::gil::gray8_view_t;
using boost::gil::rgba8c_pixel_t;
using boost::gil::rgba8_pixel_t;
using boost::gil::rgba8c_view_t;
using boost::gil::rgba8_view_t;
using boost::gil::interleaved_view;
using boost::gil::subimage_view;
using boost::gil::copy_pixels;

namespace dp
{

HWTextureAllocatorApple::HWTextureAllocatorApple()
{
  CVReturn cvRetval = CVOpenGLESTextureCacheCreate(kCFAllocatorDefault, nullptr,
                                                   [EAGLContext currentContext],
                                                   nullptr, &m_textureCache);

  CHECK_EQUAL(cvRetval, kCVReturnSuccess, ());
}

HWTextureAllocatorApple::~HWTextureAllocatorApple()
{
  CFRelease(m_textureCache);
}

CVPixelBufferRef HWTextureAllocatorApple::CVCreatePixelBuffer(uint32_t width, uint32_t height, int format)
{
  NSDictionary * attrs = [NSDictionary dictionaryWithObjectsAndKeys:
                            [NSDictionary dictionary], kCVPixelBufferIOSurfacePropertiesKey,
                            [NSNumber numberWithInt:16], kCVPixelBufferBytesPerRowAlignmentKey,
                            [NSNumber numberWithBool:YES], kCVPixelBufferOpenGLESCompatibilityKey,
                            nil];

  CFDictionaryRef attrsRef = (CFDictionaryRef)attrs;

  CVPixelBufferRef result;
  CVReturn cvRetval;
  switch (format)
  {
  case dp::RGBA8:
    cvRetval = CVPixelBufferCreate(kCFAllocatorDefault, width, height, kCVPixelFormatType_32BGRA, attrsRef, &result);
    break;
  case dp::ALPHA:
    cvRetval = CVPixelBufferCreate(kCFAllocatorDefault, width, height, kCVPixelFormatType_OneComponent8,
                                   attrsRef, &result);
    break;
  default:
    ASSERT(false, ());
    break;
  }

  CHECK_EQUAL(cvRetval, kCVReturnSuccess, ());

  return result;
}

void HWTextureAllocatorApple::CVDestroyPixelBuffer(CVPixelBufferRef buffer)
{
  CFRelease(buffer);
}

CVOpenGLESTextureRef HWTextureAllocatorApple::CVCreateTexture(CVPixelBufferRef buffer, uint32_t width, uint32_t height,
                                                              glConst layout, glConst pixelType)
{
  CVOpenGLESTextureRef texture;
  CVReturn cvRetval = CVOpenGLESTextureCacheCreateTextureFromImage(kCFAllocatorDefault, m_textureCache, buffer,
                                                                   nullptr, gl_const::GLTexture2D, layout,
                                                                   width, height, layout, pixelType, 0, &texture);

  CHECK_EQUAL(cvRetval, kCVReturnSuccess, ());

  return texture;
}

void HWTextureAllocatorApple::CVDestroyTexture(CVOpenGLESTextureRef texture)
{
  CFRelease(texture);
}

void HWTextureAllocatorApple::RiseFlushFlag()
{
  m_needFlush = true;
}

drape_ptr<HWTexture> HWTextureAllocatorApple::CreateTexture()
{
  return make_unique_dp<HWTextureApple>(make_ref<HWTextureAllocatorApple>(this));
}

void HWTextureAllocatorApple::Flush()
{
  if (m_needFlush)
  {
    CVOpenGLESTextureCacheFlush(m_textureCache, 0);
    m_needFlush = false;
  }
}

HWTextureApple::HWTextureApple(ref_ptr<HWTextureAllocatorApple> allocator)
  : m_directBuffer(nullptr)
  , m_texture(nullptr)
  , m_allocator(nullptr)
  , m_directPointer(nullptr)
{
}

HWTextureApple::~HWTextureApple()
{
  if (m_allocator == nullptr)
  {
    m_allocator->CVDestroyTexture(m_texture);
    m_allocator->CVDestroyPixelBuffer(m_directBuffer);
  }
}

void HWTextureApple::Create(Params const & params, ref_ptr<void> data)
{
  TBase::Create(params, data);

  m_allocator = params.m_allocator.downcast<HWTextureAllocatorApple>();
  m_directBuffer = m_allocator->CVCreatePixelBuffer(params.m_width, params.m_height, params.m_format);

  glConst layout, pixelType;
  UnpackFormat(params.m_format, layout, pixelType);
  m_texture = m_allocator->CVCreateTexture(m_directBuffer, params.m_width, params.m_height,
                                         layout, pixelType);

  m_textureID = CVOpenGLESTextureGetName(m_texture);
  GLFunctions::glBindTexture(m_textureID);
  GLFunctions::glTexParameter(gl_const::GLMinFilter, params.m_minFilter);
  GLFunctions::glTexParameter(gl_const::GLMagFilter, params.m_magFilter);
  GLFunctions::glTexParameter(gl_const::GLWrapS, params.m_wrapSMode);
  GLFunctions::glTexParameter(gl_const::GLWrapT, params.m_wrapTMode);

  if (data == nullptr)
    return;

  Lock();
  memcpy(m_directPointer, data.get(), m_width * m_height * GetBytesPerPixel(m_format));
  Unlock();
}

void HWTextureApple::UploadData(uint32_t x, uint32_t y, uint32_t width, uint32_t height, ref_ptr<void> data)
{
  uint8_t bytesPerPixel = GetBytesPerPixel(GetFormat());
  Lock();
  if (bytesPerPixel == 1)
  {
    gray8c_view_t srcView = interleaved_view(width, height, (gray8c_pixel_t *)data.get(), width);
    gray8_view_t dstView = interleaved_view(m_width, m_height, (gray8_pixel_t *)m_directPointer, m_width);
    gray8_view_t subDstView = subimage_view(dstView, x, y, width, height);
    copy_pixels(srcView, subDstView);
  }
  else if (bytesPerPixel == 4)
  {
    rgba8c_view_t srcView = interleaved_view(width, height,
                                             (rgba8c_pixel_t *)data.get(),
                                             width * bytesPerPixel);
    rgba8_view_t dstView = interleaved_view(m_width, m_height,
                                            (rgba8_pixel_t *)m_directPointer,
                                            m_width * bytesPerPixel);
    rgba8_view_t subDstView = subimage_view(dstView, x, y, width, height);
    copy_pixels(srcView, subDstView);
  }
  Unlock();
}

void HWTextureApple::Lock()
{
  ASSERT(m_directPointer == nullptr, ());

  CHECK_EQUAL(CVPixelBufferLockBaseAddress(m_directBuffer, 0), kCVReturnSuccess, ());

  ASSERT_EQUAL(CVPixelBufferGetBytesPerRow(m_directBuffer), m_width * GetBytesPerPixel(m_format), ());
  m_directPointer = CVPixelBufferGetBaseAddress(m_directBuffer);
}

void HWTextureApple::Unlock()
{
  ASSERT(m_directPointer != nullptr, ());
  m_directPointer = nullptr;
  CHECK_EQUAL(CVPixelBufferUnlockBaseAddress(m_directBuffer, 0), kCVReturnSuccess, ());
  m_allocator->RiseFlushFlag();
}

}
