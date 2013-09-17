#include "texture.hpp"
#include "glfunctions.hpp"

namespace
{
  glConst convert(TextureInfo::PixelType t)
  {
    if (t == TextureInfo::FullRGBA)
      return GLConst::GL8BitOnChannel;

    return GLConst::GL4BitOnChannel;
  }
}

Texture::Texture(TextureInfo const & info)
{
  Init(info, NULL);
}

Texture::Texture(TextureInfo const & info, void * data)
{
  Init(info, data);
}

void Texture::Update(uint32_t x, uint32_t y, uint32_t width, uint32_t height, void * data)
{
  GLFunctions::glTexSubImage2D(x, y, width, height, convert(m_info.m_pixelType), data);
}

uint32_t Texture::GetID() const
{
  return m_textureID;
}

void Texture::Bind()
{
  GLFunctions::glBindTexture(m_textureID);
}

TextureInfo const & Texture::GetInfo() const
{
  return m_info;
}

void Texture::Init(const TextureInfo &info, void * data)
{
  m_info = info;
  m_textureID = GLFunctions::glGenTexture();
  Bind();
  GLFunctions::glTexImage2D(m_info.m_width, m_info.m_height, convert(m_info.m_pixelType), data);
}


TextureBinding::TextureBinding(const std::string & uniformName,
                               bool isEnabled,
                               uint8_t samplerBlock,
                               WeakPointer<Texture> texture)
  : m_uniformName(uniformName)
  , m_isEnabled(isEnabled)
  , m_samplerBlock(samplerBlock)
  , m_texture(texture)
{
}

void TextureBinding::Bind(int8_t uniformLocation)
{
  if (IsEnabled() || uniformLocation == -1)
    return;

  GLFunctions::glActiveTexture(m_samplerBlock);
  m_texture->Bind();
  GLFunctions::glUniformValue(uniformLocation, (int32_t)m_texture->GetID());
}

bool TextureBinding::IsEnabled() const
{
  return m_isEnabled && !m_texture.IsNull();
}

const string & TextureBinding::GetUniformName() const
{
  return m_uniformName;
}

void TextureBinding::SetIsEnabled(bool isEnabled)
{
  m_isEnabled = isEnabled;
}

void TextureBinding::SetTexture(WeakPointer<Texture> texture)
{
  m_texture = texture;
}
