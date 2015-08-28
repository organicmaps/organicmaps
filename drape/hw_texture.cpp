#include "hw_texture.hpp"

#include "glfunctions.hpp"
#include "glextensions_list.hpp"

#include "base/math.hpp"

#if defined(OMIM_OS_IPHONE)
#include "hw_texture_ios.hpp"
#endif

#define ASSERT_ID ASSERT(GetID() != -1, ())

namespace dp
{

HWTexture::HWTexture()
  : m_width(0)
  , m_height(0)
  , m_format(UNSPECIFIED)
  , m_textureID(-1)
{
}

void HWTexture::Create(Params const & params)
{
  Create(params, nullptr);
}

void HWTexture::Create(Params const & params, ref_ptr<void> /*data*/)
{
  m_width = params.m_width;
  m_height = params.m_height;
  m_format = params.m_format;

#if defined(TRACK_GPU_MEM)
  glConst layout;
  glConst pixelType;
  UnpackFormat(format, layout, pixelType);

  uint32_t channelBitSize = 8;
  uint32_t channelCount = 4;
  if (pixelType == gl_const::GL4BitOnChannel)
    channelBitSize = 4;

  if (layout == gl_const::GLAlpha)
    channelCount = 1;

  uint32_t bitCount = channelBitSize * channelCount * m_width * m_height;
  uint32_t memSize = bitCount >> 3;
  dp::GPUMemTracker::Inst().AddAllocated("Texture", m_textureID, memSize);
  dp::GPUMemTracker::Inst().SetUsed("Texture", m_textureID, memSize);
#endif
}

TextureFormat HWTexture::GetFormat() const
{
  return m_format;
}

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
  switch (format)
  {
  case RGBA8:
    layout = gl_const::GLRGBA;
    pixelType = gl_const::GL8BitOnChannel;
    break;
  case ALPHA:
    layout = gl_const::GLAlpha;
    pixelType = gl_const::GL8BitOnChannel;
    break;
  default:
    ASSERT(false, ());
    break;
  }
}

void HWTexture::Bind() const
{
  ASSERT_ID;
  GLFunctions::glBindTexture(GetID());
}

int32_t HWTexture::GetID() const
{
  return m_textureID;
}

OpenGLHWTexture::~OpenGLHWTexture()
{
  if (m_textureID != -1)
    GLFunctions::glDeleteTexture(m_textureID);
}

void OpenGLHWTexture::Create(Params const & params, ref_ptr<void> data)
{
  TBase::Create(params, data);

  if (!GLExtensionsList::Instance().IsSupported(GLExtensionsList::TextureNPOT))
  {
    m_width = my::NextPowOf2(m_width);
    m_height = my::NextPowOf2(m_height);
  }

  m_textureID = GLFunctions::glGenTexture();
  Bind();

  glConst layout;
  glConst pixelType;
  UnpackFormat(m_format, layout, pixelType);

  GLFunctions::glTexImage2D(m_width, m_height, layout, pixelType, data.get());
  GLFunctions::glTexParameter(gl_const::GLMinFilter, params.m_minFilter);
  GLFunctions::glTexParameter(gl_const::GLMagFilter, params.m_magFilter);
  GLFunctions::glTexParameter(gl_const::GLWrapS, params.m_wrapSMode);
  GLFunctions::glTexParameter(gl_const::GLWrapT, params.m_wrapTMode);

  GLFunctions::glFlush();
  GLFunctions::glBindTexture(0);
}

void OpenGLHWTexture::UploadData(uint32_t x, uint32_t y, uint32_t width, uint32_t height, ref_ptr<void> data)
{
  ASSERT_ID;
  glConst layout;
  glConst pixelType;
  UnpackFormat(m_format, layout, pixelType);

  GLFunctions::glTexSubImage2D(x, y, width, height, layout, pixelType, data.get());
}

drape_ptr<HWTexture> OpenGLHWTextureAllocator::CreateTexture()
{
  return make_unique_dp<OpenGLHWTexture>();
}

drape_ptr<HWTextureAllocator> CreateAllocator()
{
#if defined(OMIM_OS_IPHONE)
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

}
