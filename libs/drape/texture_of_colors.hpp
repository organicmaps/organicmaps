#pragma once

#include "drape/dynamic_texture.hpp"
#include "drape/rainbow_colors.hpp"
#include "drape/texture.hpp"

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

  /// Allocate a contiguous horizontal strip of N colors (always new texels, not cached).
  /// @param[out] firstCenter, lastCenter — UV centers of first and last color.
  /// @return false if the atlas is full (caller should fall back to a single color).
  bool ReserveStrip(RainbowColors const & colors, m2::PointF & firstCenter, m2::PointF & lastCenter);

  void UploadResources(ref_ptr<dp::GraphicsContext> context, ref_ptr<Texture> texture);

private:
  using TPalette = std::map<Color, ColorResourceInfo>;

  struct PendingColor
  {
    m2::RectU m_rect;
    Color m_color;
  };

  struct StripInfo
  {
    m2::PointF m_firstCenter;
    m2::PointF m_lastCenter;
  };

  TPalette m_palette;
  TPalette m_predefinedPalette;
  std::map<RainbowColors, StripInfo> m_stripCache;
  // We have > 400 colors, no need to use buffer_vector here.
  std::vector<PendingColor> m_nodes;
  std::vector<PendingColor> m_pendingNodes;
  m2::PointU m_textureSize;
  m2::PointU m_cursor;

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
  bool ReserveStrip(RainbowColors const & colors, m2::PointF & firstCenter, m2::PointF & lastCenter);

  ~ColorTexture() override { TBase::Reset(); }

  static int GetColorSizeInPixels();

private:
  ColorPalette m_palette;
};
}  // namespace dp
