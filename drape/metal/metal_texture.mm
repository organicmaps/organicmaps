#include "drape/metal/metal_texture.hpp"
#include "drape/metal/metal_base_context.hpp"

#include "base/logging.hpp"

drape_ptr<dp::HWTextureAllocator> CreateMetalAllocator()
{
  return make_unique_dp<dp::metal::MetalTextureAllocator>();
}

ref_ptr<dp::HWTextureAllocator> GetDefaultMetalAllocator()
{
  static dp::metal::MetalTextureAllocator allocator;
  return make_ref(&allocator);
}

namespace dp
{
namespace metal
{
namespace
{
MTLPixelFormat UnpackFormat(TextureFormat format)
{
  switch (format)
  {
  case TextureFormat::RGBA8: return MTLPixelFormatRGBA8Unorm;
  case TextureFormat::Red: return MTLPixelFormatA8Unorm; // TODO: change to R8, fix shaders
  case TextureFormat::RedGreen: return MTLPixelFormatRG8Unorm;
  case TextureFormat::DepthStencil: return MTLPixelFormatDepth32Float_Stencil8;
  case TextureFormat::Depth: return MTLPixelFormatDepth32Float;
  case TextureFormat::Unspecified:
    CHECK(false, ());
    return MTLPixelFormatInvalid;
  }
  CHECK(false, ());
}
}  // namespace

drape_ptr<HWTexture> MetalTextureAllocator::CreateTexture(ref_ptr<dp::GraphicsContext> context)
{
  return make_unique_dp<MetalTexture>(make_ref(this));
}

void MetalTexture::Create(ref_ptr<dp::GraphicsContext> context, Params const & params, ref_ptr<void> data)
{
  Base::Create(context, params, data);
  ref_ptr<MetalBaseContext> metalContext = context;
  id<MTLDevice> metalDevice = metalContext->GetMetalDevice();

  MTLTextureDescriptor * texDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:UnpackFormat(params.m_format)
                                                                                      width:params.m_width
                                                                                     height:params.m_height
                                                                                  mipmapped:NO];
  texDesc.usage = MTLTextureUsageShaderRead;
  m_isMutable = params.m_isMutable;
  if (params.m_isRenderTarget)
  {
    texDesc.usage |= MTLTextureUsageRenderTarget;
    texDesc.storageMode = MTLStorageModePrivate;
    m_texture = [metalDevice newTextureWithDescriptor:texDesc];
    CHECK(m_texture != nil, ());
  }
  else
  {
    texDesc.storageMode = MTLStorageModeShared;
    m_texture = [metalDevice newTextureWithDescriptor:texDesc];
    CHECK(m_texture != nil, ());
    MTLRegion region = MTLRegionMake2D(0, 0, m_params.m_width, m_params.m_height);
    auto const rowBytes = m_params.m_width * GetBytesPerPixel(m_params.m_format);
    [m_texture replaceRegion:region mipmapLevel:0 withBytes:data.get() bytesPerRow:rowBytes];
  }
}

void MetalTexture::UploadData(ref_ptr<dp::GraphicsContext> context, uint32_t x, uint32_t y,
                              uint32_t width, uint32_t height, ref_ptr<void> data)
{
  UNUSED_VALUE(context);
  CHECK(m_isMutable, ("Upload data is avaivable only for mutable textures."));
  MTLRegion region = MTLRegionMake2D(x, y, width, height);
  auto const rowBytes = width * GetBytesPerPixel(m_params.m_format);
  [m_texture replaceRegion:region mipmapLevel:0 withBytes:data.get() bytesPerRow:rowBytes];
}

void MetalTexture::SetFilter(TextureFilter filter)
{
  m_params.m_filter = filter;
}

bool MetalTexture::Validate() const
{
  return m_texture != nil;
}
}  // namespace metal
}  // namespace dp
