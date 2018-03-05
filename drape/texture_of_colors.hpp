#pragma once

#include "drape/texture.hpp"
#include "drape/color.hpp"
#include "drape/dynamic_texture.hpp"

#include "base/buffer_vector.hpp"

#include <map>
#include <mutex>

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

  ref_ptr<Texture::ResourceInfo> ReserveResource(bool predefined, ColorKey const & key, bool & newResource);
  ref_ptr<Texture::ResourceInfo> MapResource(ColorKey const & key, bool & newResource);
  void UploadResources(ref_ptr<Texture> texture);

  void SetIsDebug(bool isDebug) { m_isDebug = isDebug; }

private:
  typedef std::map<Color, ColorResourceInfo> TPalette;

  struct PendingColor
  {
    m2::RectU m_rect;
    Color m_color;
  };

  TPalette m_palette;
  TPalette m_predefinedPalette;
  buffer_vector<PendingColor, 16> m_pendingNodes;
  m2::PointU m_textureSize;
  m2::PointU m_cursor;
  bool m_isDebug = false;
  std::mutex m_lock;
  std::mutex m_mappingLock;
};

class ColorTexture : public DynamicTexture<ColorPalette, ColorKey, Texture::Color>
{
  typedef DynamicTexture<ColorPalette, ColorKey, Texture::Color> TBase;
public:
  ColorTexture(m2::PointU const & size, ref_ptr<HWTextureAllocator> allocator)
    : m_palette(size)
  {
    TBase::TextureParams params{size, TextureFormat::RGBA8, gl_const::GLNearest, false /* m_usePixelBuffer */};
    TBase::Init(allocator, make_ref(&m_palette), params);
  }

  void ReserveColor(dp::Color const & color);

  ~ColorTexture() { TBase::Reset(); }

  static int GetColorSizeInPixels();

private:
  ColorPalette m_palette;
};
}  // namespace dp
