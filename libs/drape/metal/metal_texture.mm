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
  case TextureFormat::Red: return MTLPixelFormatA8Unorm;  // TODO: change to R8, fix shaders
  case TextureFormat::RedGreen: return MTLPixelFormatRG8Unorm;
  case TextureFormat::DepthStencil: return MTLPixelFormatDepth32Float_Stencil8;
  case TextureFormat::Depth: return MTLPixelFormatDepth32Float;
  case TextureFormat::Unspecified: CHECK(false, ()); return MTLPixelFormatInvalid;
  }
  CHECK(false, ());
}

MTLTextureDescriptor * CreateTextureDescriptor(HWTexture::Params const & params)
{
  if (params.m_layerCount > 1)
  {
    MTLTextureDescriptor * texDesc = [MTLTextureDescriptor new];
    texDesc.textureType = MTLTextureType2DArray;
    texDesc.pixelFormat = UnpackFormat(params.m_format);
    texDesc.width = params.m_width;
    texDesc.height = params.m_height;
    texDesc.arrayLength = params.m_layerCount;
    texDesc.mipmapLevelCount = 1;
    return texDesc;
  }
  return [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:UnpackFormat(params.m_format)
                                                            width:params.m_width
                                                           height:params.m_height
                                                        mipmapped:NO];
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

  MTLTextureDescriptor * texDesc = CreateTextureDescriptor(params);
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
    if (data)
    {
      auto const imageBytes = GetBytesPerPixel(params.m_format) * params.m_width * params.m_height;
      for (uint32_t layer = 0; layer < params.m_layerCount; ++layer)
      {
        void * layerData = static_cast<uint8_t *>(data.get()) + layer * imageBytes;
        UploadDataImpl(0, 0, params.m_width, params.m_height, layer, make_ref(layerData));
      }
    }
  }
}

void MetalTexture::UploadData(ref_ptr<dp::GraphicsContext> context, uint32_t x, uint32_t y, uint32_t width,
                              uint32_t height, ref_ptr<void> data)
{
  UploadData(context, x, y, width, height, 0, data);
}

void MetalTexture::UploadData(ref_ptr<dp::GraphicsContext> context, uint32_t x, uint32_t y, uint32_t width,
                              uint32_t height, uint32_t layer, ref_ptr<void> data)
{
  UNUSED_VALUE(context);
  CHECK(m_isMutable, ("Upload data is avaivable only for mutable textures."));
  UploadDataImpl(x, y, width, height, layer, data);
}

void MetalTexture::UploadDataImpl(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t layer,
                                  ref_ptr<void> data)
{
  MTLRegion region = MTLRegionMake2D(x, y, width, height);
  auto const rowBytes = width * GetBytesPerPixel(m_params.m_format);
  [m_texture replaceRegion:region mipmapLevel:0 slice:layer withBytes:data.get() bytesPerRow:rowBytes bytesPerImage:0];
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
