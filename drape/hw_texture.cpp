#include "hw_texture.hpp"

#include "glextensions_list.hpp"
#include "glfunctions.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"
#include "base/math.hpp"

#ifdef DEBUG
#include "3party/glm/glm/gtx/bit.hpp"
#endif

#if defined(OMIM_OS_IPHONE)
#include "hw_texture_ios.hpp"
#endif

#define ASSERT_ID ASSERT(GetID() != 0, ())

namespace dp
{
HWTexture::HWTexture()
  : m_width(0)
  , m_height(0)
  , m_format(UNSPECIFIED)
  , m_textureID(0)
  , m_filter(gl_const::GLLinear)
  , m_pixelBufferID(0)
  , m_pixelBufferSize(0)
  , m_pixelBufferElementSize(0)
{}

HWTexture::~HWTexture()
{
#if defined(TRACK_GPU_MEM)
  dp::GPUMemTracker::Inst().RemoveDeallocated("Texture", m_textureID);
  dp::GPUMemTracker::Inst().RemoveDeallocated("PBO", m_pixelBufferID);
#endif
}

void HWTexture::Create(Params const & params) { Create(params, nullptr); }

void HWTexture::Create(Params const & params, ref_ptr<void> /*data*/)
{
  m_width = params.m_width;
  m_height = params.m_height;
  m_format = params.m_format;
  m_filter = params.m_filter;

  uint32_t const bytesPerPixel = GetBytesPerPixel(m_format);
  if (GLFunctions::CurrentApiVersion == dp::ApiVersion::OpenGLES3 && params.m_usePixelBuffer &&
      bytesPerPixel > 0)
  {
    float const kPboPercent = 0.1f;
    m_pixelBufferElementSize = bytesPerPixel;
    m_pixelBufferSize = static_cast<uint32_t>(kPboPercent * m_width * m_height * bytesPerPixel);
  }

#if defined(TRACK_GPU_MEM)
  uint32_t const memSize = (CHAR_BIT * bytesPerPixel * m_width * m_height) >> 3;
  dp::GPUMemTracker::Inst().AddAllocated("Texture", m_textureID, memSize);
  dp::GPUMemTracker::Inst().SetUsed("Texture", m_textureID, memSize);
  if (params.m_usePixelBuffer)
  {
    dp::GPUMemTracker::Inst().AddAllocated("PBO", m_pixelBufferID, m_pixelBufferSize);
    dp::GPUMemTracker::Inst().SetUsed("PBO", m_pixelBufferID, m_pixelBufferSize);
  }
#endif
}

TextureFormat HWTexture::GetFormat() const { return m_format; }

uint32_t HWTexture::GetWidth() const
{
  ASSERT_ID;
  return m_width;
}

uint32_t HWTexture::GetHeight() const
{
  ASSERT_ID;
  return m_height;
}

float HWTexture::GetS(uint32_t x) const
{
  ASSERT_ID;
  return x / (float)m_width;
}

float HWTexture::GetT(uint32_t y) const
{
  ASSERT_ID;
  return y / (float)m_height;
}

void HWTexture::UnpackFormat(TextureFormat format, glConst & layout, glConst & pixelType)
{
  // Now we support only 1-byte-per-channel textures.
  pixelType = gl_const::GL8BitOnChannel;

  switch (format)
  {
  case RGBA8:
    layout = gl_const::GLRGBA;
    break;
  case ALPHA:
    // On OpenGL ES3 GLAlpha is not supported, we use GLRed instead.
    layout = GLFunctions::CurrentApiVersion == dp::ApiVersion::OpenGLES2 ? gl_const::GLAlpha
                                                                         : gl_const::GLRed;
    break;
  case RED_GREEN:
    // On OpenGL ES2 2-channel textures are not supported.
    layout = GLFunctions::CurrentApiVersion == dp::ApiVersion::OpenGLES2 ? gl_const::GLRGBA
                                                                         : gl_const::GLRedGreen;
    break;
  default: ASSERT(false, ()); break;
  }
}

void HWTexture::Bind() const
{
  ASSERT_ID;
  if (m_textureID != 0)
    GLFunctions::glBindTexture(GetID());
}

void HWTexture::SetFilter(glConst filter)
{
  if (m_filter != filter)
  {
    m_filter = filter;
    GLFunctions::glTexParameter(gl_const::GLMinFilter, m_filter);
    GLFunctions::glTexParameter(gl_const::GLMagFilter, m_filter);
  }
}

uint32_t HWTexture::GetID() const { return m_textureID; }

OpenGLHWTexture::~OpenGLHWTexture()
{
  if (m_textureID != 0)
    GLFunctions::glDeleteTexture(m_textureID);

  if (m_pixelBufferID != 0)
    GLFunctions::glDeleteBuffer(m_pixelBufferID);
}

void OpenGLHWTexture::Create(Params const & params, ref_ptr<void> data)
{
  TBase::Create(params, data);

  ASSERT(glm::isPowerOfTwo(static_cast<int>(m_width)), (m_width));
  ASSERT(glm::isPowerOfTwo(static_cast<int>(m_height)), (m_height));

  m_textureID = GLFunctions::glGenTexture();
  Bind();

  glConst layout;
  glConst pixelType;
  UnpackFormat(m_format, layout, pixelType);

  GLFunctions::glTexImage2D(m_width, m_height, layout, pixelType, data.get());
  GLFunctions::glTexParameter(gl_const::GLMinFilter, params.m_filter);
  GLFunctions::glTexParameter(gl_const::GLMagFilter, params.m_filter);
  GLFunctions::glTexParameter(gl_const::GLWrapS, params.m_wrapSMode);
  GLFunctions::glTexParameter(gl_const::GLWrapT, params.m_wrapTMode);

  if (m_pixelBufferSize > 0)
  {
    m_pixelBufferID = GLFunctions::glGenBuffer();
    GLFunctions::glBindBuffer(m_pixelBufferID, gl_const::GLPixelBufferWrite);
    GLFunctions::glBufferData(gl_const::GLPixelBufferWrite, m_pixelBufferSize, nullptr,
                              gl_const::GLDynamicDraw);
    GLFunctions::glBindBuffer(0, gl_const::GLPixelBufferWrite);
  }

  GLFunctions::glFlush();
  GLFunctions::glBindTexture(0);
}

void OpenGLHWTexture::UploadData(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                                 ref_ptr<void> data)
{
  ASSERT_ID;
  glConst layout;
  glConst pixelType;
  UnpackFormat(m_format, layout, pixelType);

  uint32_t const mappingSize = height * width * m_pixelBufferElementSize;
  if (m_pixelBufferID != 0 && m_pixelBufferSize != 0 && m_pixelBufferSize >= mappingSize)
  {
    ASSERT_GREATER(m_pixelBufferElementSize, 0, ());
    GLFunctions::glBindBuffer(m_pixelBufferID, gl_const::GLPixelBufferWrite);
    GLFunctions::glBufferSubData(gl_const::GLPixelBufferWrite, mappingSize, data.get(), 0);
    GLFunctions::glTexSubImage2D(x, y, width, height, layout, pixelType, nullptr);
    GLFunctions::glBindBuffer(0, gl_const::GLPixelBufferWrite);
  }
  else
  {
    GLFunctions::glTexSubImage2D(x, y, width, height, layout, pixelType, data.get());
  }
}

drape_ptr<HWTexture> OpenGLHWTextureAllocator::CreateTexture()
{
  return make_unique_dp<OpenGLHWTexture>();
}

drape_ptr<HWTextureAllocator> CreateAllocator()
{
  if (GLFunctions::CurrentApiVersion == dp::ApiVersion::OpenGLES3)
  {
    return make_unique_dp<OpenGLHWTextureAllocator>();
  }
#if defined(OMIM_OS_IPHONE) && !defined(OMIM_OS_IPHONE_SIMULATOR)
  return make_unique_dp<HWTextureAllocatorApple>();
#else
  return make_unique_dp<OpenGLHWTextureAllocator>();
#endif
}

ref_ptr<HWTextureAllocator> GetDefaultAllocator()
{
  static OpenGLHWTextureAllocator s_allocator;
  return make_ref<HWTextureAllocator>(&s_allocator);
}
}  // namespace dp
