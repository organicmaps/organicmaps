#pragma once

#include "drape/dynamic_texture.hpp"
#include "drape/glyph_manager.hpp"
#include "drape/pointers.hpp"

#include <map>
#include <mutex>
#include <vector>

namespace dp
{
class GlyphPacker
{
public:
  explicit GlyphPacker(m2::PointU const & size);

  bool PackGlyph(uint32_t width, uint32_t height, m2::RectU & rect);
  bool CanBePacked(uint32_t glyphsCount, uint32_t width, uint32_t height) const;
  m2::RectF MapTextureCoords(m2::RectU const & pixelRect) const;
  bool IsFull() const;
  m2::PointU const & GetSize() const { return m_size; }

private:
  m2::PointU m_size = m2::PointU(0, 0);
  m2::PointU m_cursor = m2::PointU(0, 0);
  uint32_t m_yStep = 0;
  bool m_isFull = false;
};

class GlyphKey : public Texture::Key
{
public:
  explicit GlyphKey(strings::UniChar unicodePoint)
    : m_unicodePoint(unicodePoint)
  {}

  Texture::ResourceType GetType() const override { return Texture::ResourceType::Glyph; }
  strings::UniChar GetUnicodePoint() const { return m_unicodePoint; }

  bool operator<(GlyphKey const & g) const
  {
    return m_unicodePoint < g.m_unicodePoint;
  }

private:
  strings::UniChar m_unicodePoint;
};

class GlyphInfo : public Texture::ResourceInfo
{
public:
  GlyphInfo(m2::RectF const & texRect, GlyphMetrics const & metrics)
    : ResourceInfo(texRect)
    , m_metrics(metrics)
  {}
  ~GlyphInfo() override = default;

  Texture::ResourceType GetType() const override { return Texture::ResourceType::Glyph; }
  GlyphMetrics const & GetMetrics() const { return m_metrics; }

private:
  GlyphMetrics m_metrics;
};

class GlyphIndex
{
public:
  GlyphIndex(m2::PointU const & size, ref_ptr<GlyphManager> mng);
  ~GlyphIndex();

  // This function can return nullptr.
  ref_ptr<Texture::ResourceInfo> MapResource(GlyphKey const & key, bool & newResource);
  std::vector<ref_ptr<Texture::ResourceInfo>> MapResources(std::vector<GlyphKey> const & keys, bool & hasNewResources);
  void UploadResources(ref_ptr<dp::GraphicsContext> context, ref_ptr<Texture> texture);

  bool CanBeGlyphPacked(uint32_t glyphsCount) const;

  // ONLY for unit-tests. DO NOT use this function anywhere else.
  size_t GetPendingNodesCount();

private:
  GlyphPacker m_packer;
  ref_ptr<GlyphManager> m_mng;

  using ResourceMapping = std::map<GlyphKey, GlyphInfo>;
  using PendingNode = std::pair<m2::RectU, Glyph>;
  using PendingNodes = std::vector<PendingNode>;

  ResourceMapping m_index;
  PendingNodes m_pendingNodes;
  std::mutex m_mutex;
};

class FontTexture : public DynamicTexture<GlyphIndex, GlyphKey, Texture::ResourceType::Glyph>
{
public:
  FontTexture(m2::PointU const & size, ref_ptr<GlyphManager> glyphMng, ref_ptr<HWTextureAllocator> allocator)
    : m_index(size, glyphMng)
  {
    DynamicTextureParams const params{size, TextureFormat::Alpha, TextureFilter::Linear, true /* m_usePixelBuffer */};
    Init(allocator, make_ref(&m_index), params);
  }

  ~FontTexture() override { Reset(); }

  std::vector<ref_ptr<ResourceInfo>> FindResources(std::vector<GlyphKey> const & keys, bool & hasNewResources) const
  {
    ASSERT(m_indexer != nullptr, ());
    return m_indexer->MapResources(keys, hasNewResources);
  }

  bool HasEnoughSpace(uint32_t newKeysCount) const override
  {
    return m_index.CanBeGlyphPacked(newKeysCount);
  }

private:
  GlyphIndex m_index;
};
}  // namespace dp
