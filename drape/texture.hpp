#pragma once

#include "pointers.hpp"

#include "../std/string.hpp"

struct TextureInfo
{
  enum PixelType
  {
    FullRGBA,  // 8 bit on channel
    HalfRGBA   // 4 bit on channel
  };

  TextureInfo()
    : m_width(0), m_height(0), m_pixelType(FullRGBA)
  {
  }

  TextureInfo(uint32_t width, uint32_t height, PixelType type)
    : m_width(width), m_height(height), m_pixelType(type)
  {
  }

  uint32_t m_width;
  uint32_t m_height;
  PixelType m_pixelType;
};

class Texture
{
public:
  Texture(TextureInfo const & info);
  Texture(TextureInfo const & info, void * data);

  void Update(uint32_t x, uint32_t y, uint32_t width, uint32_t height, void * data);

  uint32_t GetID() const;
  void Bind();

  TextureInfo const & GetInfo() const;

  bool operator<(const Texture & other) const
  {
    return m_textureID < other.m_textureID;
  }

private:
  void Init(TextureInfo const & info, void * data);

private:
  uint32_t m_textureID;
  TextureInfo m_info;
};

class TextureBinding
{
public:
  TextureBinding(const string & uniformName, bool isEnabled, uint8_t samplerBlock, WeakPointer<Texture> texture);

  void Bind(int8_t uniformLocation);
  bool IsEnabled() const;
  const string & GetUniformName() const;
  void SetIsEnabled(bool isEnabled);
  void SetTexture(WeakPointer<Texture> texture);

  bool operator<(const TextureBinding & other) const
  {
    return m_isEnabled < other.m_isEnabled
        || m_samplerBlock < other.m_samplerBlock
        || m_texture < other.m_texture;
  }

private:
  string m_uniformName;
  bool m_isEnabled;
  uint8_t m_samplerBlock;
  WeakPointer<Texture> m_texture;
};
