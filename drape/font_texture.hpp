#pragma once

#include "drape/dynamic_texture.hpp"
#include "drape/glyph_generator.hpp"
#include "drape/glyph_manager.hpp"
#include "drape/pointers.hpp"

#include <atomic>
#include <map>
#include <memory>
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
  GlyphKey(strings::UniChar unicodePoint, int fixedSize)
    : m_unicodePoint(unicodePoint)
    , m_fixedSize(fixedSize)
  {}

  Texture::ResourceType GetType() const { return Texture::Glyph; }
  strings::UniChar GetUnicodePoint() const { return m_unicodePoint; }
  int GetFixedSize() const { return m_fixedSize; }

  bool operator<(GlyphKey const & g) const
  {
    if (m_unicodePoint == g.m_unicodePoint)
      return m_fixedSize < g.m_fixedSize;
    return m_unicodePoint < g.m_unicodePoint;
  }

private:
  strings::UniChar m_unicodePoint;
  int m_fixedSize;
};

class GlyphInfo : public Texture::ResourceInfo
{
  using Base = Texture::ResourceInfo;

public:
  GlyphInfo(m2::RectF const & texRect, GlyphManager::GlyphMetrics const & metrics)
    : Base(texRect)
    , m_metrics(metrics)
  {}
  ~GlyphInfo() override = default;

  Texture::ResourceType GetType() const override { return Texture::Glyph; }
  GlyphManager::GlyphMetrics const & GetMetrics() const { return m_metrics; }

private:
  GlyphManager::GlyphMetrics m_metrics;
};

class GlyphIndex : public GlyphGenerator::Listener
{
public:
  GlyphIndex(m2::PointU const & size, ref_ptr<GlyphManager> mng,
             ref_ptr<GlyphGenerator> generator);
  ~GlyphIndex() override;

  // This function can return nullptr.
  ref_ptr<Texture::ResourceInfo> MapResource(GlyphKey const & key, bool & newResource);
  std::vector<ref_ptr<Texture::ResourceInfo>> MapResources(std::vector<GlyphKey> const & keys,
                                                           bool & hasNewResources);
  void UploadResources(ref_ptr<Texture> texture);

  bool CanBeGlyphPacked(uint32_t glyphsCount) const;

  uint32_t GetAbsentGlyphsCount(strings::UniString const & text, int fixedHeight) const;

  // ONLY for unit-tests. DO NOT use this function anywhere else.
  size_t GetPendingNodesCount();

private:
  ref_ptr<Texture::ResourceInfo> MapResource(GlyphKey const & key, bool & newResource,
                                             GlyphGenerator::GlyphGenerationData & generationData);
  void OnCompleteGlyphGeneration(GlyphGenerator::GlyphGenerationDataArray && glyphs) override;

  GlyphPacker m_packer;
  ref_ptr<GlyphManager> m_mng;
  ref_ptr<GlyphGenerator> m_generator;

  using ResourceMapping = std::map<GlyphKey, GlyphInfo>;
  using PendingNode = std::pair<m2::RectU, GlyphManager::Glyph>;
  using PendingNodes = std::vector<PendingNode>;

  ResourceMapping m_index;
  PendingNodes m_pendingNodes;
  std::mutex m_mutex;
};

class FontTexture : public DynamicTexture<GlyphIndex, GlyphKey, Texture::Glyph>
{
  using TBase = DynamicTexture<GlyphIndex, GlyphKey, Texture::Glyph>;
public:
  FontTexture(m2::PointU const & size, ref_ptr<GlyphManager> glyphMng,
              ref_ptr<GlyphGenerator> glyphGenerator, ref_ptr<HWTextureAllocator> allocator)
    : m_index(size, glyphMng, glyphGenerator)
  {
    TBase::TextureParams params{size, TextureFormat::ALPHA,
                                gl_const::GLLinear, true /* m_usePixelBuffer */};
    TBase::Init(allocator, make_ref(&m_index), params);
  }

  ~FontTexture() override { TBase::Reset(); }

  std::vector<ref_ptr<ResourceInfo>> FindResources(std::vector<GlyphKey> const & keys,
                                                   bool & hasNewResources)
  {
    ASSERT(m_indexer != nullptr, ());
    return m_indexer->MapResources(keys, hasNewResources);
  }

  bool HasEnoughSpace(uint32_t newKeysCount) const override
  {
    return m_index.CanBeGlyphPacked(newKeysCount);
  }

  uint32_t GetAbsentGlyphsCount(strings::UniString const & text, int fixedHeight) const
  {
    return m_index.GetAbsentGlyphsCount(text, fixedHeight);
  }

private:
  GlyphIndex m_index;
};
}  // namespace dp
