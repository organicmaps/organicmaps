#include "texture.hpp"

#include "glfunctions.hpp"
#include "glextensions_list.hpp"

#include "../base/math.hpp"

#define ASSERT_ID ASSERT(GetID() != -1, ())

Texture::Texture()
  : m_textureID(-1)
{
}

Texture::~Texture()
{
  if (m_textureID != -1)
    GLFunctions::glDeleteTexture(m_textureID);
}

void Texture::Create(uint32_t width, uint32_t height, TextureFormat format)
{
  Create(width, height, format, MakeStackRefPointer<void>(NULL));
}

void Texture::Create(uint32_t width, uint32_t height, TextureFormat format, RefPointer<void> data)
{
  m_width = width;
  m_height = height;
  if (!GLExtensionsList::Instance().IsSupported(GLExtensionsList::TextureNPOT))
  {
    m_width = my::NextPowOf2(width);
    m_height = my::NextPowOf2(height);
  }

  m_textureID = GLFunctions::glGenTexture();
  GLFunctions::glBindTexture(m_textureID);

  glConst layout;
  glConst pixelType;
  UnpackFormat(format, layout, pixelType);

  GLFunctions::glTexImage2D(m_width, m_height, layout, pixelType, data.GetRaw());
  SetFilterParams(GLConst::GLLinear, GLConst::GLLinear);
  SetWrapMode(GLConst::GLClampToEdge, GLConst::GLClampToEdge);
}

void Texture::SetFilterParams(glConst minFilter, glConst magFilter)
{
  ASSERT_ID;
  GLFunctions::glTexParameter(GLConst::GLMinFilter, minFilter);
  GLFunctions::glTexParameter(GLConst::GLMagFilter, magFilter);
}

void Texture::SetWrapMode(glConst sMode, glConst tMode)
{
  ASSERT_ID;
  GLFunctions::glTexParameter(GLConst::GLWrapS, sMode);
  GLFunctions::glTexParameter(GLConst::GLWrapT, tMode);
}

void Texture::UploadData(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                         TextureFormat format, RefPointer<void> data)
{
  ASSERT_ID;
  glConst layout;
  glConst pixelType;

  UnpackFormat(format, layout, pixelType);

  GLFunctions::glTexSubImage2D(x, y, width, height, layout, pixelType, data.GetRaw());
}

uint32_t Texture::GetWidth() const
{
  ASSERT_ID;
  return m_width;
}

uint32_t Texture::GetHeight() const
{
  ASSERT_ID;
  return m_height;
}

float Texture::GetS(uint32_t x) const
{
  ASSERT_ID;
  return x / (float)m_width;
}

float Texture::GetT(uint32_t y) const
{
  ASSERT_ID;
  return y / (float)m_height;
}

void Texture::Bind() const
{
  ASSERT_ID;
  GLFunctions::glBindTexture(GetID());
}

void Texture::UnpackFormat(Texture::TextureFormat format, glConst & layout, glConst & pixelType)
{
  bool requiredFormat = GLExtensionsList::Instance().IsSupported(GLExtensionsList::RequiredInternalFormat);
  switch (format) {
  case RGBA8:
    layout = requiredFormat ? GLConst::GLRGBA8 : GLConst::GLRGBA;
    pixelType = GLConst::GL8BitOnChannel;
    break;
  case RGBA4:
    layout = requiredFormat ? GLConst::GLRGBA4 : GLConst::GLRGBA;
    pixelType = GLConst::GL4BitOnChannel;
    break;
  default:
    ASSERT(false, ());
    break;
  }
}

int32_t Texture::GetID() const
{
  return m_textureID;
}
