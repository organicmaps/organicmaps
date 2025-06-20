#include "drape/hw_texture_ios.hpp"
#include "drape/gl_functions.hpp"
#include "drape/gl_includes.hpp"

#include "base/logging.hpp"

#import <QuartzCore/CAEAGLLayer.h>

#import <Foundation/NSDictionary.h>
#import <Foundation/NSValue.h>

#include <boost/integer_traits.hpp>

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

CVPixelBufferRef HWTextureAllocatorApple::CVCreatePixelBuffer(uint32_t width, uint32_t height,
                                                              dp::TextureFormat format)
{
  NSDictionary * attrs = [NSDictionary dictionaryWithObjectsAndKeys:
                            [NSDictionary dictionary], kCVPixelBufferIOSurfacePropertiesKey,
                            [NSNumber numberWithInt:16], kCVPixelBufferBytesPerRowAlignmentKey,
                            [NSNumber numberWithBool:YES], kCVPixelBufferOpenGLESCompatibilityKey,
                            nil];

  CFDictionaryRef attrsRef = (__bridge CFDictionaryRef)attrs;

  CVPixelBufferRef result = nullptr;
  CVReturn cvRetval = 0;
  switch (format)
  {
  case dp::TextureFormat::RGBA8:
    cvRetval = CVPixelBufferCreate(kCFAllocatorDefault, width, height, kCVPixelFormatType_32BGRA,
                                   attrsRef, &result);
    break;
  case dp::TextureFormat::Red:
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

drape_ptr<HWTexture> HWTextureAllocatorApple::CreateTexture(ref_ptr<dp::GraphicsContext> context)
{
  UNUSED_VALUE(context);
  return make_unique_dp<HWTextureApple>(make_ref(this));
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
  if (m_allocator != nullptr)
  {
    m_allocator->CVDestroyTexture(m_texture);
    m_allocator->CVDestroyPixelBuffer(m_directBuffer);
  }
}

void HWTextureApple::Create(ref_ptr<dp::GraphicsContext> context, Params const & params, ref_ptr<void> data)
{
  TBase::Create(context, params, data);

  m_allocator = m_params.m_allocator;
  m_directBuffer = m_allocator->CVCreatePixelBuffer(m_params.m_width, m_params.m_height, params.m_format);

  glConst layout, pixelType;
  UnpackFormat(context, params.m_format, layout, pixelType);
  m_texture = m_allocator->CVCreateTexture(m_directBuffer, params.m_width, params.m_height,
                                         layout, pixelType);

  m_textureID = CVOpenGLESTextureGetName(m_texture);
  GLFunctions::glBindTexture(m_textureID);
  auto const f = DecodeTextureFilter(m_params.m_filter);
  GLFunctions::glTexParameter(gl_const::GLMinFilter, f);
  GLFunctions::glTexParameter(gl_const::GLMagFilter, f);
  GLFunctions::glTexParameter(gl_const::GLWrapS, DecodeTextureWrapping(m_params.m_wrapSMode));
  GLFunctions::glTexParameter(gl_const::GLWrapT, DecodeTextureWrapping(m_params.m_wrapTMode));

  if (data == nullptr)
    return;

  Lock();
  memcpy(m_directPointer, data.get(), m_params.m_width * m_params.m_height * GetBytesPerPixel(m_params.m_format));
  Unlock();
}

void HWTextureApple::UploadData(ref_ptr<dp::GraphicsContext> context, uint32_t x, uint32_t y,
                                uint32_t width, uint32_t height, ref_ptr<void> data)
{
  uint8_t bytesPerPixel = GetBytesPerPixel(GetFormat());
  Lock();
  if (bytesPerPixel == 1)
  {
    gray8c_view_t srcView = interleaved_view(width, height, (gray8c_pixel_t *)data.get(), width);
    gray8_view_t dstView = interleaved_view(m_params.m_width, m_params.m_height,
                                            (gray8_pixel_t *)m_directPointer, m_params.m_width);
    gray8_view_t subDstView = subimage_view(dstView, x, y, width, height);
    copy_pixels(srcView, subDstView);
  }
  else if (bytesPerPixel == 4)
  {
    rgba8c_view_t srcView = interleaved_view(width, height,
                                             (rgba8c_pixel_t *)data.get(),
                                             width * bytesPerPixel);
    rgba8_view_t dstView = interleaved_view(m_params.m_width, m_params.m_height,
                                            (rgba8_pixel_t *)m_directPointer,
                                            m_params.m_width * bytesPerPixel);
    rgba8_view_t subDstView = subimage_view(dstView, x, y, width, height);
    copy_pixels(srcView, subDstView);
  }
  Unlock();
}

void HWTextureApple::Bind(ref_ptr<dp::GraphicsContext> context) const
{
  UNUSED_VALUE(context);
  ASSERT(Validate(), ());
  if (m_textureID != 0)
    GLFunctions::glBindTexture(GetID());
}
  
void HWTextureApple::SetFilter(TextureFilter filter)
{
  ASSERT(Validate(), ());
  if (m_params.m_filter != filter)
  {
    m_params.m_filter = filter;
    auto const f = DecodeTextureFilter(m_params.m_filter);
    GLFunctions::glTexParameter(gl_const::GLMinFilter, f);
    GLFunctions::glTexParameter(gl_const::GLMagFilter, f);
  }
}
  
bool HWTextureApple::Validate() const
{
  return GetID() != 0;
}
  
void HWTextureApple::Lock()
{
  ASSERT(m_directPointer == nullptr, ());

  CHECK_EQUAL(CVPixelBufferLockBaseAddress(m_directBuffer, 0), kCVReturnSuccess, ());

  ASSERT_EQUAL(CVPixelBufferGetBytesPerRow(m_directBuffer),
               m_params.m_width * GetBytesPerPixel(m_params.m_format), ());
  m_directPointer = CVPixelBufferGetBaseAddress(m_directBuffer);
}

void HWTextureApple::Unlock()
{
  ASSERT(m_directPointer != nullptr, ());
  m_directPointer = nullptr;
  CHECK_EQUAL(CVPixelBufferUnlockBaseAddress(m_directBuffer, 0), kCVReturnSuccess, ());
  m_allocator->RiseFlushFlag();
}
}  // namespace dp
