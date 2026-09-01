#pragma once

#include "drape/dynamic_texture.hpp"
#include "drape/pointers.hpp"
#include "drape/texture.hpp"

#include "base/buffer_vector.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <cmath>
#include <map>
#include <mutex>

namespace dp
{
uint32_t constexpr kMaxStipplePenLength = 512;  /// @todo Should be equal with kStippleTextureWidth?

// Based on ./data/patterns.txt, the most of patterns have 2 entries (4 entries for triangles pattern).
using PenPatternT = buffer_vector<uint16_t, 2>;

inline uint16_t PatternFloat2Pixel(double d)
{
  uint16_t const px = std::lround(d);
  // ASSERT(px > 0, (d));
  return px > 0 ? px : 1;
}

inline bool IsTrianglePattern(PenPatternT const & p)
{
  return p.size() == 4;
}

class StipplePenKey : public Texture::Key
{
public:
  StipplePenKey() = default;
  explicit StipplePenKey(PenPatternT const & pattern) : m_pattern(pattern) {}

  virtual Texture::ResourceType GetType() const { return Texture::ResourceType::StipplePen; }

  bool operator<(StipplePenKey const & rhs) const { return m_pattern < rhs.m_pattern; }
  bool operator==(StipplePenKey const & rhs) const { return m_pattern == rhs.m_pattern; }

  PenPatternT m_pattern;
};

class StipplePenRasterizator
{
public:
  explicit StipplePenRasterizator(StipplePenKey const & key);

  m2::PointU GetSize() const { return {m_pixelLength, m_height}; }

  void Rasterize(uint8_t * buffer) const;

private:
  void RasterizeDash(uint8_t * buffer) const;
  void RasterizeTriangle(uint8_t * buffer) const;
  void ClonePattern(uint8_t * pixels) const;

private:
  StipplePenKey m_key;
  uint32_t m_patternLength, m_pixelLength;
  uint32_t m_height;
};

class StipplePenResourceInfo : public Texture::ResourceInfo
{
public:
  StipplePenResourceInfo(m2::RectF const & texRect, m2::PointU const & pixelSize)
    : Texture::ResourceInfo(texRect)
    , m_pixelSize(pixelSize)
  {}

  virtual Texture::ResourceType GetType() const { return Texture::ResourceType::StipplePen; }

  m2::PointU GetMaskPixelSize() const { return m_pixelSize; }

private:
  m2::PointU m_pixelSize;
};

class StipplePenPacker
{
public:
  explicit StipplePenPacker(m2::PointU const & canvasSize);

  m2::RectU PackResource(m2::PointU const & size);
  m2::RectF MapTextureCoords(m2::RectU const & pixelRect) const;

private:
  m2::PointU m_canvasSize;
  uint32_t m_currentRow;
};

class StipplePenIndex
{
public:
  explicit StipplePenIndex(m2::PointU const & canvasSize) : m_packer(canvasSize) {}
  /// @param[out] newResource Needed for the generic DynamicTexture code.
  /// @{
  // Called from TextureManager::Init and fills m_predefinedResourceMapping, no need in m_mappingLock.
  ref_ptr<Texture::ResourceInfo> ReserveResource(bool predefined, StipplePenKey const & key, bool & newResource);
  // Checks m_predefinedResourceMapping, fills m_resourceMapping, locks m_mappingLock.
  ref_ptr<Texture::ResourceInfo> MapResource(StipplePenKey const & key, bool & newResource);
  /// @}
  void UploadResources(ref_ptr<dp::GraphicsContext> context, ref_ptr<Texture> texture);

private:
  // std::unordered_map can be better here
  typedef std::map<StipplePenKey, StipplePenResourceInfo> TResourceMapping;
  typedef std::pair<m2::RectU, StipplePenRasterizator> TPendingNode;
  typedef std::vector<TPendingNode> TPendingNodes;

  // Initialized once via ReserveResource.
  TResourceMapping m_predefinedResourceMapping;
  // Filled async via MapResource, protected with m_mappingLock.
  TResourceMapping m_resourceMapping;

  TPendingNodes m_pendingNodes;
  StipplePenPacker m_packer;

  std::mutex m_lock;
  std::mutex m_mappingLock;

  bool m_uploadCalled = false;
};

class StipplePenTexture : public DynamicTexture<StipplePenIndex, StipplePenKey, Texture::ResourceType::StipplePen>
{
  using TBase = DynamicTexture<StipplePenIndex, StipplePenKey, Texture::ResourceType::StipplePen>;

public:
  StipplePenTexture(m2::PointU const & size, ref_ptr<HWTextureAllocator> allocator) : m_index(size)
  {
    TBase::DynamicTextureParams params{size, TextureFormat::Red, TextureFilter::Nearest, false /* m_usePixelBuffer */};
    TBase::Init(allocator, make_ref(&m_index), params);
  }

  ~StipplePenTexture() override { TBase::Reset(); }

  void ReservePattern(PenPatternT const & pattern);

private:
  StipplePenIndex m_index;
};
}  // namespace dp
