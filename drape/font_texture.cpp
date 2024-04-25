#include "drape/font_texture.hpp"

#include "drape/pointers.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <functional>
#include <iterator>

namespace dp
{
GlyphPacker::GlyphPacker(const m2::PointU & size)
  : m_size(size)
{}

bool GlyphPacker::PackGlyph(uint32_t width, uint32_t height, m2::RectU & rect)
{
  ASSERT_LESS(width, m_size.x, ());
  ASSERT_LESS(height, m_size.y, ());

  if (m_cursor.x + width > m_size.x)
  {
    m_cursor.x = 0;
    m_cursor.y += m_yStep;
    m_yStep = 0;
  }

  if (m_cursor.y + height > m_size.y)
  {
    m_isFull = true;
    return false;
  }

  rect = m2::RectU(m_cursor.x, m_cursor.y,
                   m_cursor.x + width, m_cursor.y + height);

  m_cursor.x += width;
  m_yStep = std::max(height, m_yStep);
  return true;
}

bool GlyphPacker::CanBePacked(uint32_t glyphsCount, uint32_t width, uint32_t height) const
{
  uint32_t x = m_cursor.x;
  uint32_t y = m_cursor.y;
  uint32_t step = m_yStep;
  for (uint32_t i = 0; i < glyphsCount; i++)
  {
    if (x + width > m_size.x)
    {
      x = 0;
      y += step;
    }

    if (y + height > m_size.y)
      return false;

    x += width;
    step = std::max(height, step);
  }
  return true;
}

m2::RectF GlyphPacker::MapTextureCoords(const m2::RectU & pixelRect) const
{
  auto const width = static_cast<float>(m_size.x);
  auto const height = static_cast<float>(m_size.y);

  // Half-pixel offset to eliminate artefacts on fetching from texture.
  float offset = 0.0f;
  if (pixelRect.SizeX() != 0 && pixelRect.SizeY() != 0)
    offset = 0.5f;

  return {(pixelRect.minX() + offset) / width,
          (pixelRect.minY() + offset) / height,
          (pixelRect.maxX() - offset) / width,
          (pixelRect.maxY() - offset) / height};
}

bool GlyphPacker::IsFull() const { return m_isFull; }

GlyphIndex::GlyphIndex(m2::PointU const & size, ref_ptr<GlyphManager> mng)
  : m_packer(size)
  , m_mng(mng)
{
  ASSERT(m_mng != nullptr, ());

  // Cache predefined glyphs.
  bool newResource = false;
  uint32_t constexpr kPredefinedGlyphsCount = 128;
  for (uint32_t i = 0; i < kPredefinedGlyphsCount; ++i)
  {
    auto const key = GlyphKey(i);

    MapResource(key, newResource);
  }
}

GlyphIndex::~GlyphIndex()
{
  std::lock_guard lock(m_mutex);
  for (auto & node : m_pendingNodes)
    node.second.m_image.Destroy();
  m_pendingNodes.clear();
}

std::vector<ref_ptr<Texture::ResourceInfo>> GlyphIndex::MapResources(std::vector<GlyphKey> const & keys,
                                                                     bool & hasNewResources)
{
  std::vector<ref_ptr<Texture::ResourceInfo>> info;
  info.reserve(keys.size());

  hasNewResources = false;
  for (auto const & glyphKey : keys)
  {
    bool newResource = false;
    auto result = MapResource(glyphKey, newResource);
    hasNewResources |= newResource;
    info.push_back(std::move(result));
  }

  return info;
}

ref_ptr<Texture::ResourceInfo> GlyphIndex::MapResource(GlyphKey const & key, bool & newResource)
{
  newResource = false;
  auto it = m_index.find(key);
  if (it != m_index.end())
    return make_ref(&it->second);

  newResource = true;

  Glyph glyph = m_mng->GetGlyph(key.GetUnicodePoint());
  m2::RectU r;
  if (!m_packer.PackGlyph(glyph.m_image.m_width, glyph.m_image.m_height, r))
  {
    glyph.m_image.Destroy();
    if (glyph.m_metrics.m_isValid)
    {
      LOG(LWARNING, ("Glyphs packer could not pack a glyph", key.GetUnicodePoint(),
        "w =", glyph.m_image.m_width, "h =", glyph.m_image.m_height,
        "packerSize =", m_packer.GetSize()));
    }

    auto const & invalidGlyph = m_mng->GetInvalidGlyph();
    auto invalidGlyphIndex = m_index.find(GlyphKey(invalidGlyph.m_code));
    if (invalidGlyphIndex != m_index.end())
    {
      newResource = false;
      return make_ref(&invalidGlyphIndex->second);
    }

    return nullptr;
  }

  auto res = m_index.emplace(key, GlyphInfo(m_packer.MapTextureCoords(r), glyph.m_metrics));
  ASSERT(res.second, ());

  {
    std::lock_guard lock(m_mutex);
    m_pendingNodes.emplace_back(r, std::move(glyph));
  }

  return make_ref(&res.first->second);
}

bool GlyphIndex::CanBeGlyphPacked(uint32_t glyphsCount) const
{
  if (glyphsCount == 0)
    return true;

  if (m_packer.IsFull())
    return false;

  float constexpr kGlyphScalar = 1.5f;
  auto const baseSize = static_cast<uint32_t>(m_mng->GetBaseGlyphHeight() * kGlyphScalar);
  return m_packer.CanBePacked(glyphsCount, baseSize, baseSize);
}

size_t GlyphIndex::GetPendingNodesCount()
{
  std::lock_guard lock(m_mutex);
  return m_pendingNodes.size();
}

void GlyphIndex::UploadResources(ref_ptr<GraphicsContext> context, ref_ptr<Texture> texture)
{
  PendingNodes pendingNodes;
  {
    std::lock_guard lock(m_mutex);
    if (m_pendingNodes.empty())
      return;
    m_pendingNodes.swap(pendingNodes);
  }

  for (auto it = pendingNodes.begin(); it != pendingNodes.end();)
  {
    m_mng->MarkGlyphReady(it->second);

    if (!it->second.m_image.m_data)
      it = pendingNodes.erase(it);
    else
      ++it;
  }

  if (pendingNodes.empty())
    return;

  for (auto & [rect, glyph] : pendingNodes)
  {
    m2::PointU const zeroPoint = rect.LeftBottom();
    if (glyph.m_image.m_width == 0 || glyph.m_image.m_height == 0 || rect.SizeX() == 0 || rect.SizeY() == 0)
    {
      LOG(LWARNING, ("Glyph skipped", glyph.m_code));
      continue;
    }
    ASSERT_EQUAL(glyph.m_image.m_width, rect.SizeX(), ());
    ASSERT_EQUAL(glyph.m_image.m_height, rect.SizeY(), ());

    uint8_t * srcMemory = SharedBufferManager::GetRawPointer(glyph.m_image.m_data);
    texture->UploadData(context, zeroPoint.x, zeroPoint.y, rect.SizeX(), rect.SizeY(), make_ref(srcMemory));

    glyph.m_image.Destroy();
  }
}
}  // namespace dp
