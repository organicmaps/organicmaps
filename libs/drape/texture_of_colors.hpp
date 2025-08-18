#pragma once

#include "drape/color.hpp"
#include "drape/dynamic_texture.hpp"
#include "drape/texture.hpp"

#include "base/buffer_vector.hpp"

#include <map>
#include <mutex>

namespace dp
{
class ColorKey : public Texture::Key
{
public:
  explicit ColorKey(Color color) : Texture::Key(), m_color(color) {}
  virtual Texture::ResourceType GetType() const { return Texture::ResourceType::Color; }

  Color m_color;
};

class ColorResourceInfo : public Texture::ResourceInfo
{
public:
  explicit ColorResourceInfo(m2::RectF const & texRect) : Texture::ResourceInfo(texRect) {}
  virtual Texture::ResourceType GetType() const { return Texture::ResourceType::Color; }
};

class ColorPalette
{
public:
  explicit ColorPalette(m2::PointU const & canvasSize);

  ref_ptr<Texture::ResourceInfo> ReserveResource(bool predefined, ColorKey const & key, bool & newResource);
  ref_ptr<Texture::ResourceInfo> MapResource(ColorKey const & key, bool & newResource);
  void UploadResources(ref_ptr<dp::GraphicsContext> context, ref_ptr<Texture> texture);

  void SetIsDebug(bool isDebug) { m_isDebug = isDebug; }

private:
  using TPalette = std::map<Color, ColorResourceInfo>;

  struct PendingColor
  {
    m2::RectU m_rect;
    Color m_color;
  };

  TPalette m_palette;
  TPalette m_predefinedPalette;
  // We have > 400 colors, no need to use buffer_vector here.
  std::vector<PendingColor> m_nodes;
  std::vector<PendingColor> m_pendingNodes;
  m2::PointU m_textureSize;
  m2::PointU m_cursor;
  bool m_isDebug = false;
  std::mutex m_lock;
  std::mutex m_mappingLock;
};

class ColorTexture : public DynamicTexture<ColorPalette, ColorKey, Texture::ResourceType::Color>
{
  using TBase = DynamicTexture<ColorPalette, ColorKey, Texture::ResourceType::Color>;

public:
  ColorTexture(m2::PointU const & size, ref_ptr<HWTextureAllocator> allocator) : m_palette(size)
  {
    TBase::DynamicTextureParams params{size, TextureFormat::RGBA8, TextureFilter::Nearest,
                                       false /* m_usePixelBuffer */};
    TBase::Init(allocator, make_ref(&m_palette), params);
  }

  void ReserveColor(dp::Color const & color);

  ~ColorTexture() override { TBase::Reset(); }

  static int GetColorSizeInPixels();

private:
  ColorPalette m_palette;
};
}  // namespace dp
