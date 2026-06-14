#include "drape/hw_texture.hpp"

#include "drape/drape_global.hpp"
#include "drape/gl_functions.hpp"
#include "drape/utils/gpu_mem_tracker.hpp"

#include "base/bits.hpp"
#include "base/logging.hpp"

#include "std/target_os.hpp"

#include <algorithm>

#if defined(OMIM_OS_IPHONE)
#include "drape/hw_texture_ios.hpp"
#endif

#if defined(OMIM_METAL_AVAILABLE)
extern drape_ptr<dp::HWTextureAllocator> CreateMetalAllocator();
extern ref_ptr<dp::HWTextureAllocator> GetDefaultMetalAllocator();
#endif

extern drape_ptr<dp::HWTextureAllocator> CreateVulkanAllocator();
extern ref_ptr<dp::HWTextureAllocator> GetDefaultVulkanAllocator();

namespace dp
{
std::string DebugPrint(HWTexture::Params const & p)
{
  std::ostringstream ss;
  ss << "Width = " << p.m_width << "; Height = " << p.m_height << "; Format = " << DebugPrint(p.m_format)
     << "; IsRenderTarget = " << p.m_isRenderTarget;
  return ss.str();
}

void UnpackFormat(ref_ptr<dp::GraphicsContext> context, TextureFormat format, glConst & layout, glConst & pixelType)
{
  auto const apiVersion = context->GetApiVersion();
  CHECK(apiVersion == dp::ApiVersion::OpenGLES3, ());

  switch (format)
  {
  case TextureFormat::RGBA8:
    layout = gl_const::GLRGBA;
    pixelType = gl_const::GL8BitOnChannel;
    return;

  case TextureFormat::Red:
    layout = gl_const::GLRed;
    pixelType = gl_const::GL8BitOnChannel;
    return;

  case TextureFormat::RedGreen:
    layout = gl_const::GLRedGreen;
    pixelType = gl_const::GL8BitOnChannel;
    return;

  case TextureFormat::DepthStencil:
    layout = gl_const::GLDepthStencil;
    pixelType = gl_const::GLUnsignedInt24_8Type;
    return;

  case TextureFormat::Depth:
    layout = gl_const::GLDepthComponent;
    pixelType = gl_const::GLUnsignedIntType;
    return;

  case TextureFormat::Unspecified: CHECK(false, ()); return;
  }
  UNREACHABLE();
}

glConst DecodeTextureFilter(TextureFilter filter)
{
  switch (filter)
  {
  case TextureFilter::Linear: return gl_const::GLLinear;
  case TextureFilter::Nearest: return gl_const::GLNearest;
  }
  UNREACHABLE();
}

glConst DecodeTextureWrapping(TextureWrapping wrapping)
{
  switch (wrapping)
  {
  case TextureWrapping::ClampToEdge: return gl_const::GLClampToEdge;
  case TextureWrapping::Repeat: return gl_const::GLRepeat;
  }
  UNREACHABLE();
}

bool CanBuildMipmaps(HWTexture::Params const & params, bool hasData)
{
  return params.m_useMipmaps && params.m_layerCount == 1 && hasData;
}

uint32_t GetMipLevelsCount(uint32_t width, uint32_t height)
{
  return 1 + bits::FloorLog(std::max(width, height));
}

std::vector<MipLevel> BuildMipmapLevels(uint8_t const * level0Data, uint32_t width, uint32_t height,
                                        uint8_t bytesPerPixel)
{
  std::vector<MipLevel> levels;
  levels.reserve(GetMipLevelsCount(width, height) - 1);

  uint32_t sw = width, sh = height;  // current source-level dimensions
  while (sw > 1 || sh > 1)
  {
    // The source is level 0, then each previously generated level.
    uint8_t const * const src = levels.empty() ? level0Data : levels.back().m_data.data();
    uint32_t const dw = std::max(sw >> 1, 1u);
    uint32_t const dh = std::max(sh >> 1, 1u);

    MipLevel level{dw, dh, std::vector<uint8_t>(static_cast<size_t>(dw) * dh * bytesPerPixel)};
    uint8_t * const dst = level.m_data.data();
    for (uint32_t y = 0; y < dh; ++y)
    {
      uint32_t const sy0 = std::min(y * 2, sh - 1);
      uint32_t const sy1 = std::min(sy0 + 1, sh - 1);
      for (uint32_t x = 0; x < dw; ++x)
      {
        uint32_t const sx0 = std::min(x * 2, sw - 1);
        uint32_t const sx1 = std::min(sx0 + 1, sw - 1);
        uint8_t const * const p[4] = {src + (static_cast<size_t>(sy0) * sw + sx0) * bytesPerPixel,
                                      src + (static_cast<size_t>(sy0) * sw + sx1) * bytesPerPixel,
                                      src + (static_cast<size_t>(sy1) * sw + sx0) * bytesPerPixel,
                                      src + (static_cast<size_t>(sy1) * sw + sx1) * bytesPerPixel};
        uint8_t * const d = dst + (static_cast<size_t>(y) * dw + x) * bytesPerPixel;
        if (bytesPerPixel == 4)
        {
          // Alpha-weighted RGB so fully transparent texels (whose RGB is meaningless) don't darken edges.
          uint32_t const a = p[0][3] + p[1][3] + p[2][3] + p[3][3];
          for (int c = 0; c < 3; ++c)
          {
            d[c] = a == 0
                     ? 0
                     : static_cast<uint8_t>(
                           (p[0][c] * p[0][3] + p[1][c] * p[1][3] + p[2][c] * p[2][3] + p[3][c] * p[3][3] + a / 2) / a);
          }
          d[3] = static_cast<uint8_t>((a + 2) / 4);
        }
        else
        {
          for (uint8_t c = 0; c < bytesPerPixel; ++c)
            d[c] = static_cast<uint8_t>((p[0][c] + p[1][c] + p[2][c] + p[3][c] + 2) / 4);
        }
      }
    }

    levels.push_back(std::move(level));
    sw = dw;
    sh = dh;
  }
  return levels;
}

namespace
{
// Minification filter for a texture: trilinear across mip levels when present, otherwise the plain filter.
// Magnification always uses the plain filter (there's nothing to interpolate above level 0).
glConst GetGLMinFilter(TextureFilter filter, bool useMipmaps)
{
  if (!useMipmaps)
    return DecodeTextureFilter(filter);
  return filter == TextureFilter::Linear ? gl_const::GLLinearMipmapLinear : gl_const::GLNearestMipmapNearest;
}
}  // namespace

HWTexture::~HWTexture()
{
#if defined(TRACK_GPU_MEM)
  dp::GPUMemTracker::Inst().RemoveDeallocated("Texture", m_textureID);
  dp::GPUMemTracker::Inst().RemoveDeallocated("PBO", m_pixelBufferID);
#endif
}

void HWTexture::Create(ref_ptr<dp::GraphicsContext> context, Params const & params)
{
  Create(context, params, nullptr);
}

void HWTexture::Create(ref_ptr<dp::GraphicsContext>, Params const & params, ref_ptr<void> data)
{
  m_params = params;
  // Single source of truth for "does this texture get a mip chain": every backend calls Base::Create
  // first and then just reads m_params.m_useMipmaps (for the descriptor, upload and sampler/min filter).
  m_params.m_useMipmaps = CanBuildMipmaps(params, data != nullptr);

#if defined(TRACK_GPU_MEM)
  uint32_t const memSize = (CHAR_BIT * bytesPerPixel * m_params.m_width * m_params.m_height) >> 3;
  dp::GPUMemTracker::Inst().AddAllocated("Texture", m_textureID, memSize);
  dp::GPUMemTracker::Inst().SetUsed("Texture", m_textureID, memSize);
  if (params.m_usePixelBuffer)
  {
    dp::GPUMemTracker::Inst().AddAllocated("PBO", m_pixelBufferID, m_pixelBufferSize);
    dp::GPUMemTracker::Inst().SetUsed("PBO", m_pixelBufferID, m_pixelBufferSize);
  }
#endif
}

TextureFormat HWTexture::GetFormat() const
{
  return m_params.m_format;
}

uint32_t HWTexture::GetWidth() const
{
  ASSERT(Validate(), ());
  return m_params.m_width;
}

uint32_t HWTexture::GetHeight() const
{
  ASSERT(Validate(), ());
  return m_params.m_height;
}

float HWTexture::GetS(uint32_t x) const
{
  ASSERT(Validate(), ());
  return x / static_cast<float>(m_params.m_width);
}

float HWTexture::GetT(uint32_t y) const
{
  ASSERT(Validate(), ());
  return y / static_cast<float>(m_params.m_height);
}

uint32_t HWTexture::GetID() const
{
  return m_textureID;
}

uint32_t HWTexture::GetTarget() const
{
  return m_target;
}

OpenGLHWTexture::~OpenGLHWTexture()
{
  if (m_textureID != 0)
    GLFunctions::glDeleteTexture(m_textureID);

  if (m_pixelBufferID != 0)
    GLFunctions::glDeleteBuffer(m_pixelBufferID);
}

void OpenGLHWTexture::Create(ref_ptr<dp::GraphicsContext> context, Params const & params, ref_ptr<void> data)
{
  Base::Create(context, params, data);

  m_target = params.m_layerCount > 1 ? gl_const::GLTexture2DArray : gl_const::GLTexture2D;

  CHECK(context && m_textureID == 0 && m_pixelBufferID == 0, ());

  if (params.m_usePixelBuffer && params.m_layerCount == 1)
  {
    m_pixelBufferElementSize = GetBytesPerPixel(m_params.m_format);
    if (m_pixelBufferElementSize > 0)
    {
      // Buffer size = 10%
      m_pixelBufferSize = static_cast<uint32_t>(0.1 * m_params.m_width * m_params.m_height * m_pixelBufferElementSize);
    }
  }

  m_textureID = GLFunctions::glGenTexture();
  Bind(context);

  UnpackFormat(context, m_params.m_format, m_unpackedLayout, m_unpackedPixelType);

  auto const f = DecodeTextureFilter(m_params.m_filter);
  if (m_params.m_layerCount > 1)
  {
    GLFunctions::glTexImage2DArray(m_params.m_width, m_params.m_height, m_params.m_layerCount, m_unpackedLayout,
                                   m_unpackedPixelType, data.get());
  }
  else
  {
    GLFunctions::glTexImage2D(m_params.m_width, m_params.m_height, m_unpackedLayout, m_unpackedPixelType, data.get());

    // Upload the CPU-built mip chain as explicit levels.
    if (m_params.m_useMipmaps)
    {
      auto const levels = BuildMipmapLevels(static_cast<uint8_t const *>(data.get()), m_params.m_width,
                                            m_params.m_height, GetBytesPerPixel(m_params.m_format));
      for (size_t i = 0; i < levels.size(); ++i)
      {
        GLFunctions::glTexImage2D(static_cast<int>(levels[i].m_width), static_cast<int>(levels[i].m_height),
                                  m_unpackedLayout, m_unpackedPixelType, levels[i].m_data.data(),
                                  static_cast<int>(i + 1));
      }
      GLFunctions::glTexParameter(gl_const::GLTextureMaxLevel, static_cast<glConst>(levels.size()), m_target);
    }
  }
  GLFunctions::glTexParameter(gl_const::GLMinFilter, GetGLMinFilter(m_params.m_filter, m_params.m_useMipmaps),
                              m_target);
  GLFunctions::glTexParameter(gl_const::GLMagFilter, f, m_target);
  GLFunctions::glTexParameter(gl_const::GLWrapS, DecodeTextureWrapping(m_params.m_wrapSMode), m_target);
  GLFunctions::glTexParameter(gl_const::GLWrapT, DecodeTextureWrapping(m_params.m_wrapTMode), m_target);

  if (m_pixelBufferSize > 0)
  {
    m_pixelBufferID = GLFunctions::glGenBuffer();
    GLFunctions::glBindBuffer(m_pixelBufferID, gl_const::GLPixelBufferWrite);
    GLFunctions::glBufferData(gl_const::GLPixelBufferWrite, m_pixelBufferSize, nullptr, gl_const::GLDynamicDraw);
    GLFunctions::glBindBuffer(0, gl_const::GLPixelBufferWrite);
  }

  GLFunctions::glBindTexture(0, m_target);
  GLFunctions::glFlush();
}

void OpenGLHWTexture::UploadData(ref_ptr<dp::GraphicsContext> context, uint32_t x, uint32_t y, uint32_t width,
                                 uint32_t height, ref_ptr<void> data)
{
  ASSERT(Validate(), ());

  uint32_t const mappingSize = height * width * m_pixelBufferElementSize;
  if (m_pixelBufferID != 0 && m_pixelBufferSize >= mappingSize)
  {
    ASSERT_GREATER(mappingSize, 0, ());
    ASSERT(m_params.m_layerCount == 1, ());
    GLFunctions::glBindBuffer(m_pixelBufferID, gl_const::GLPixelBufferWrite);
    GLFunctions::glBufferSubData(gl_const::GLPixelBufferWrite, mappingSize, data.get(), 0);

    GLFunctions::glTexSubImage2D(x, y, width, height, m_unpackedLayout, m_unpackedPixelType, nullptr);
    GLFunctions::glBindBuffer(0, gl_const::GLPixelBufferWrite);
  }
  else
  {
    Bind(context);
    if (m_params.m_layerCount > 1)
    {
      uint8_t * pixelData = static_cast<uint8_t *>(data.get());
      for (int layer = 0; layer < int(m_params.m_layerCount); ++layer)
      {
        GLFunctions::glTexSubImage2DArray(x, y, layer, width, height, m_unpackedLayout, m_unpackedPixelType,
                                          pixelData + layer * mappingSize);
      }
    }
    else
    {
      GLFunctions::glTexSubImage2D(x, y, width, height, m_unpackedLayout, m_unpackedPixelType, data.get());
    }
    GLFunctions::glBindTexture(0, m_target);
    GLFunctions::glFlush();
  }
}

void OpenGLHWTexture::UploadData(ref_ptr<dp::GraphicsContext> context, uint32_t x, uint32_t y, uint32_t width,
                                 uint32_t height, uint32_t layer, ref_ptr<void> data)
{
  ASSERT(Validate(), ());
  Bind(context);
  GLFunctions::glTexSubImage2DArray(x, y, layer, width, height, m_unpackedLayout, m_unpackedPixelType, data.get());
  GLFunctions::glBindTexture(0, m_target);
  GLFunctions::glFlush();
}

void OpenGLHWTexture::Bind(ref_ptr<dp::GraphicsContext> context) const
{
  UNUSED_VALUE(context);
  ASSERT(Validate(), ());
  if (m_textureID != 0)
    GLFunctions::glBindTexture(GetID(), m_target);
}

void OpenGLHWTexture::SetFilter(TextureFilter filter)
{
  ASSERT(Validate(), ());
  if (m_params.m_filter != filter)
  {
    m_params.m_filter = filter;
    GLFunctions::glTexParameter(gl_const::GLMinFilter, GetGLMinFilter(filter, m_params.m_useMipmaps), m_target);
    GLFunctions::glTexParameter(gl_const::GLMagFilter, DecodeTextureFilter(filter), m_target);
  }
}

bool OpenGLHWTexture::Validate() const
{
  return GetID() != 0;
}

drape_ptr<HWTexture> OpenGLHWTextureAllocator::CreateTexture(ref_ptr<dp::GraphicsContext> context)
{
  UNUSED_VALUE(context);
  return make_unique_dp<OpenGLHWTexture>();
}

void OpenGLHWTextureAllocator::Flush()
{
  GLFunctions::glFlush();
}

drape_ptr<HWTextureAllocator> CreateAllocator(ref_ptr<dp::GraphicsContext> context)
{
  CHECK(context != nullptr, ());
  auto const apiVersion = context->GetApiVersion();
  if (apiVersion == dp::ApiVersion::Metal)
  {
#if defined(OMIM_METAL_AVAILABLE)
    return CreateMetalAllocator();
#endif
    CHECK(false, ("Metal rendering is supported now only on iOS."));
    return nullptr;
  }

  if (apiVersion == dp::ApiVersion::Vulkan)
    return CreateVulkanAllocator();

  if (apiVersion == dp::ApiVersion::OpenGLES3)
    return make_unique_dp<OpenGLHWTextureAllocator>();

#if defined(OMIM_OS_IPHONE)
  return make_unique_dp<HWTextureAllocatorApple>();
#else
  return make_unique_dp<OpenGLHWTextureAllocator>();
#endif
}

ref_ptr<HWTextureAllocator> GetDefaultAllocator(ref_ptr<dp::GraphicsContext> context)
{
  CHECK(context != nullptr, ());
  auto const apiVersion = context->GetApiVersion();
  if (apiVersion == dp::ApiVersion::Metal)
  {
#if defined(OMIM_METAL_AVAILABLE)
    return GetDefaultMetalAllocator();
#endif
    CHECK(false, ("Metal rendering is supported now only on iOS."));
    return nullptr;
  }

  if (apiVersion == dp::ApiVersion::Vulkan)
    return GetDefaultVulkanAllocator();

  static OpenGLHWTextureAllocator s_allocator;
  return make_ref<HWTextureAllocator>(&s_allocator);
}
}  // namespace dp
