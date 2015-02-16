#pragma once

#include "drape/pointers.hpp"
#include "drape/texture.hpp"
#include "drape/glyph_manager.hpp"
#include "drape/dynamic_texture.hpp"

#include "std/map.hpp"
#include "std/vector.hpp"
#include "std/string.hpp"

namespace dp
{

class GlyphPacker
{
public:
  GlyphPacker(m2::PointU const & size);

  bool PackGlyph(uint32_t width, uint32_t height, m2::RectU & rect);
  m2::RectF MapTextureCoords(m2::RectU const & pixelRect) const;
  bool IsFull() const;

private:
  m2::PointU m_size = m2::PointU(0, 0);
  m2::PointU m_cursor = m2::PointU(0, 0);
  uint32_t m_yStep = 0;
  bool m_isFull = false;
};

class GlyphKey : public Texture::Key
{
public:
  GlyphKey(strings::UniChar unicodePoint) : m_unicodePoint(unicodePoint) {}

  Texture::ResourceType GetType() const { return Texture::Glyph; }
  strings::UniChar GetUnicodePoint() const { return m_unicodePoint; }

private:
  strings::UniChar m_unicodePoint;
};

class GlyphInfo : public Texture::ResourceInfo
{
  typedef Texture::ResourceInfo TBase;
public:
  GlyphInfo(m2::RectF const & texRect, GlyphManager::GlyphMetrics const & metrics)
    : TBase(texRect)
    , m_metrics(metrics)
  {
  }

  virtual Texture::ResourceType GetType() const { return Texture::Glyph; }
  GlyphManager::GlyphMetrics const & GetMetrics() const { return m_metrics; }

private:
  GlyphManager::GlyphMetrics m_metrics;
};

class GlyphIndex
{
public:
  GlyphIndex(m2::PointU size, RefPointer<GlyphManager> mng);
  ~GlyphIndex();

  /// can return nullptr
  RefPointer<Texture::ResourceInfo> MapResource(GlyphKey const & key, bool & newResource);
  void UploadResources(RefPointer<Texture> texture);

  glConst GetMinFilter() const { return gl_const::GLLinear; }
  glConst GetMagFilter() const { return gl_const::GLLinear; }

private:
  GlyphPacker m_packer;
  RefPointer<GlyphManager> m_mng;

  typedef map<strings::UniChar, GlyphInfo> TResourceMapping;
  typedef pair<m2::RectU, GlyphManager::Glyph> TPendingNode;
  typedef vector<TPendingNode> TPendingNodes;

  TResourceMapping m_index;
  TPendingNodes m_pendingNodes;
  threads::Mutex m_lock;
};

class FontTexture : public DynamicTexture<GlyphIndex, GlyphKey, Texture::Glyph>
{
  typedef DynamicTexture<GlyphIndex, GlyphKey, Texture::Glyph> TBase;
public:
  FontTexture(m2::PointU const & size, RefPointer<GlyphManager> glyphMng)
    : m_index(size, glyphMng)
  {
    TBase::TextureParams params;
    params.m_size = size;
    params.m_format = TextureFormat::ALPHA;
    params.m_minFilter = gl_const::GLLinear;
    params.m_magFilter = gl_const::GLLinear;

    vector<uint8_t> initData(params.m_size.x * params.m_size.y, 0);
    TBase::Init(MakeStackRefPointer(&m_index), params, MakeStackRefPointer<void>(initData.data()));
  }

  ~FontTexture() { TBase::Reset(); }

private:
  GlyphIndex m_index;
};

}
