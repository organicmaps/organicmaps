#pragma once

#include "texture.hpp"

#include "../base/buffer_vector.hpp"

namespace dp
{

class ColorKey : public Texture::Key
{
public:
  ColorKey(uint32_t color) : Texture::Key(), m_color(color) {}
  virtual Texture::ResourceType GetType() const { return Texture::Color; }
  uint32_t GetColor() const { return m_color; }
  void SetColor(uint32_t color) { m_color = color; }
private:
  uint32_t m_color;
};

class ColorResourceInfo : public Texture::ResourceInfo
{
public:
  ColorResourceInfo(m2::RectF const & texRect) : Texture::ResourceInfo(texRect) { }
  virtual Texture::ResourceType GetType() const { return Texture::Color; }
};

class ColorPalette
{
public:
  ColorPalette(m2::PointU const & canvasSize);
  ~ColorPalette();
  ColorResourceInfo const * MapResource(ColorKey const & key);
  void UploadResources(RefPointer<Texture> texture);
private:
  typedef MasterPointer<ColorResourceInfo> TResourcePtr;
  typedef map<uint32_t, TResourcePtr> TPalette;

  TPalette m_palette;
  vector<uint32_t> m_pendingNodes;
  m2::PointU m_textureSize;
  uint32_t m_curY;
  uint32_t m_curX;
};

}
