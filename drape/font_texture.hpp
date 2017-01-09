#pragma once

#include "drape/pointers.hpp"
#include "drape/texture.hpp"
#include "drape/glyph_manager.hpp"
#include "drape/dynamic_texture.hpp"

#include "std/atomic.hpp"
#include "std/condition_variable.hpp"
#include "std/list.hpp"
#include "std/map.hpp"
#include "std/vector.hpp"
#include "std/string.hpp"
#include "std/thread.hpp"

namespace dp
{

class GlyphPacker
{
public:
  GlyphPacker(m2::PointU const & size);

  bool PackGlyph(uint32_t width, uint32_t height, m2::RectU & rect);
  bool CanBePacked(uint32_t glyphsCount, uint32_t width, uint32_t height) const;
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
  typedef Texture::ResourceInfo TBase;
public:
  GlyphInfo(m2::RectF const & texRect, GlyphManager::GlyphMetrics const & metrics)
    : TBase(texRect)
    , m_metrics(metrics)
  {}

  virtual Texture::ResourceType GetType() const { return Texture::Glyph; }
  GlyphManager::GlyphMetrics const & GetMetrics() const { return m_metrics; }

private:
  GlyphManager::GlyphMetrics m_metrics;
};

class GlyphGenerator
{
public:
  using TCompletionHandler = function<void(m2::RectU const &, GlyphManager::Glyph const &)>;

  struct GlyphGenerationData
  {
    m2::RectU m_rect;
    GlyphManager::Glyph m_glyph;

    GlyphGenerationData(m2::RectU const & rect, GlyphManager::Glyph const & glyph)
      : m_rect(rect), m_glyph(glyph)
    {}
  };

  GlyphGenerator(ref_ptr<GlyphManager> mng, TCompletionHandler const & completionHandler);
  ~GlyphGenerator();

  void GenerateGlyph(m2::RectU const & rect, GlyphManager::Glyph const & glyph);

  bool IsSuspended() const;

private:
  static void Routine(GlyphGenerator * generator);
  void WaitForGlyph(list<GlyphGenerationData> & queue);

  ref_ptr<GlyphManager> m_mng;
  TCompletionHandler m_completionHandler;

  list<GlyphGenerationData> m_queue;
  mutable mutex m_queueLock;

  atomic<bool> m_isRunning;
  condition_variable m_condition;
  bool m_isSuspended;
  thread m_thread;
};

class GlyphIndex
{
public:
  GlyphIndex(m2::PointU size, ref_ptr<GlyphManager> mng);
  ~GlyphIndex();

  // This function can return nullptr.
  ref_ptr<Texture::ResourceInfo> MapResource(GlyphKey const & key, bool & newResource);
  void UploadResources(ref_ptr<Texture> texture);

  bool CanBeGlyphPacked(uint32_t glyphsCount) const;

  bool HasAsyncRoutines() const;

  uint32_t GetAbsentGlyphsCount(strings::UniString const & text, int fixedHeight) const;

  // ONLY for unit-tests. DO NOT use this function anywhere else.
  size_t GetPendingNodesCount();

private:
  void OnGlyphGenerationCompletion(m2::RectU const & rect, GlyphManager::Glyph const & glyph);

  GlyphPacker m_packer;
  ref_ptr<GlyphManager> m_mng;
  unique_ptr<GlyphGenerator> m_generator;

  typedef map<GlyphKey, GlyphInfo> TResourceMapping;
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
  FontTexture(m2::PointU const & size, ref_ptr<GlyphManager> glyphMng, ref_ptr<HWTextureAllocator> allocator)
    : m_index(size, glyphMng)
  {
    TBase::TextureParams params;
    params.m_size = size;
    params.m_format = TextureFormat::ALPHA;
    params.m_filter = gl_const::GLLinear;

    vector<uint8_t> initData(params.m_size.x * params.m_size.y, 0);
    TBase::Init(allocator, make_ref(&m_index), params, make_ref(initData.data()));
  }

  ~FontTexture() { TBase::Reset(); }

  bool HasEnoughSpace(uint32_t newKeysCount) const override
  {
    return m_index.CanBeGlyphPacked(newKeysCount);
  }

  bool HasAsyncRoutines() const override
  {
    return m_index.HasAsyncRoutines();
  }

  uint32_t GetAbsentGlyphsCount(strings::UniString const & text, int fixedHeight) const
  {
    return m_index.GetAbsentGlyphsCount(text, fixedHeight);
  }

private:
  GlyphIndex m_index;
};

}
