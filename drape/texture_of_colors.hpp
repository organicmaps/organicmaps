#pragma once

#include "texture.hpp"

#include "../base/buffer_vector.hpp"

namespace dp
{

class ColorKey : public Texture::Key
{
public:
  virtual Texture::ResourceType GetType() const { return Texture::Color; }
  uint32_t GetColor() const { return m_color; }
  uint32_t m_color;
};

class ColorResourceInfo : public Texture::ResourceInfo
{
public:
  ColorResourceInfo(m2::RectF const & texRect) : Texture::ResourceInfo(texRect) { }
  virtual Texture::ResourceType GetType() const { return Texture::Color; }
};

class TextureOfColors
{
public:
  TextureOfColors();
};

class ColorPalette
{
public:
  ColorPalette(m2::PointU const & canvasSize);
  ~ColorPalette() { delete m_info; }
  ColorResourceInfo const * MapResource(ColorKey const & key);
  void UploadResources(RefPointer<Texture> texture);
  void AddData(void const * data, uint32_t size);
public:
  void Move(uint32_t step);
  typedef map<uint32_t, uint32_t> TPalette;
  typedef pair<TPalette::iterator, bool> TInserted;
  TPalette m_palette;
  vector<uint32_t> m_pendingNodes;
  m2::PointU m_textureSize;
  uint32_t m_curY;
  uint32_t m_curX;
  ColorResourceInfo * m_info;
};

}
