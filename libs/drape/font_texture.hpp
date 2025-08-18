#pragma once

#include "drape/dynamic_texture.hpp"
#include "drape/glyph_manager.hpp"
#include "drape/pointers.hpp"

#include <map>
#include <mutex>
#include <utility>  // std::tie
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

class GlyphKey
  : public GlyphFontAndId
  , public Texture::Key
{
public:
  Texture::ResourceType GetType() const override { return Texture::ResourceType::Glyph; }
};

// TODO(AB): Make Texture::ResourceInfo non-abstract and use it here directly.
class GlyphInfo : public Texture::ResourceInfo
{
public:
  explicit GlyphInfo(m2::RectF const & texRect) : ResourceInfo(texRect) {}

  Texture::ResourceType GetType() const override { return Texture::ResourceType::Glyph; }
};

class GlyphIndex
{
public:
  GlyphIndex(m2::PointU const & size, ref_ptr<GlyphManager> mng);
  ~GlyphIndex();

  // This function can return nullptr.
  ref_ptr<Texture::ResourceInfo> MapResource(GlyphFontAndId const & key, bool & newResource);
  std::vector<ref_ptr<Texture::ResourceInfo>> MapResources(TGlyphs const & keys, bool & hasNewResources);
  void UploadResources(ref_ptr<dp::GraphicsContext> context, ref_ptr<Texture> texture);

  bool CanBeGlyphPacked(uint32_t glyphsCount) const;

  // ONLY for unit-tests. DO NOT use this function anywhere else.
  size_t GetPendingNodesCount();

private:
  GlyphPacker m_packer;
  ref_ptr<GlyphManager> m_mng;

  using ResourceMapping = std::map<GlyphFontAndId, GlyphInfo>;
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
    DynamicTextureParams const params{size, TextureFormat::Red, TextureFilter::Linear, true /* m_usePixelBuffer */};
    Init(allocator, make_ref(&m_index), params);
  }

  ~FontTexture() override { Reset(); }

  ref_ptr<ResourceInfo> MapResource(GlyphFontAndId const & key, bool & hasNewResources) const
  {
    ASSERT(m_indexer != nullptr, ());
    return m_indexer->MapResource(key, hasNewResources);
  }

  bool HasEnoughSpace(uint32_t newKeysCount) const override { return m_index.CanBeGlyphPacked(newKeysCount); }

private:
  GlyphIndex m_index;
};
}  // namespace dp
