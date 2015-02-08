#pragma once

#include "drape/texture.hpp"
#include "drape/color.hpp"
#include "drape/dynamic_texture.hpp"

#include "base/buffer_vector.hpp"
#include "base/mutex.hpp"

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

  RefPointer<Texture::ResourceInfo> MapResource(ColorKey const & key, bool & newResource);
  void UploadResources(RefPointer<Texture> texture);
  glConst GetMinFilter() const;
  glConst GetMagFilter() const;

  void SetIsDebug(bool isDebug) { m_isDebug = isDebug; }

private:
  void MoveCursor();

private:
  typedef map<Color, ColorResourceInfo> TPalette;

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
  threads::Mutex m_lock;
};

class ColorTexture : public DynamicTexture<ColorPalette, ColorKey, Texture::Color>
{
  typedef DynamicTexture<ColorPalette, ColorKey, Texture::Color> TBase;
public:
  ColorTexture(m2::PointU const & size)
    : m_pallete(size)
  {
    TBase::TextureParams params;
    params.m_size = size;
    params.m_format = TextureFormat::RGBA8;
    params.m_minFilter = gl_const::GLNearest;
    params.m_magFilter = gl_const::GLNearest;

    TBase::Init(MakeStackRefPointer(&m_pallete), params);
  }

  ~ColorTexture() { TBase::Reset(); }

private:
  ColorPalette m_pallete;
};

}
