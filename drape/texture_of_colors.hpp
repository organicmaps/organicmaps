#pragma once

#include "texture.hpp"
#include "color.hpp"

#include "../base/buffer_vector.hpp"

namespace dp
{

class ColorKey : public Texture::Key
{
public:
  ColorKey(Color color) : Texture::Key(), m_color(color) {}
  virtual Texture::ResourceType GetType() const { return Texture::Color; }

  Color m_color;
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

  void SetIsDebug(bool isDebug) { m_isDebug = isDebug; }

private:
  void MoveCursor();

private:
  typedef MasterPointer<ColorResourceInfo> TResourcePtr;
  typedef map<Color, TResourcePtr> TPalette;

  struct PendingColor
  {
    m2::RectU m_rect;
    Color m_color;
  };

  TPalette m_palette;
  buffer_vector<PendingColor, 16> m_pendingNodes;
  m2::PointU m_textureSize;
  m2::PointU m_cursor;
  bool m_isDebug = false;
};

}
