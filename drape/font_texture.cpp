#include "drape/font_texture.hpp"
#include "drape/pointers.hpp"
#include "3party/stb_image/stb_image.h"

#include "platform/platform.hpp"
#include "coding/reader.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"
#include "base/stl_add.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"
#include "std/map.hpp"
#include "std/bind.hpp"

#include <boost/gil/algorithm.hpp>
#include <boost/gil/typedefs.hpp>

using boost::gil::gray8c_pixel_t;
using boost::gil::gray8_pixel_t;
using boost::gil::gray8c_view_t;
using boost::gil::gray8_view_t;
using boost::gil::interleaved_view;
using boost::gil::subimage_view;
using boost::gil::copy_pixels;

typedef gray8_view_t view_t;
typedef gray8c_view_t const_view_t;
typedef gray8_pixel_t pixel_t;
typedef gray8c_pixel_t const_pixel_t;

namespace dp
{

GlyphPacker::GlyphPacker(const m2::PointU & size)
  : m_size(size)
{
}

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
  m_yStep = max(height, m_yStep);
  return true;
}

m2::RectF GlyphPacker::MapTextureCoords(const m2::RectU & pixelRect) const
{
  float fWidth = static_cast<float>(m_size.x);
  float fHeight = static_cast<float>(m_size.y);
  return m2::RectF(pixelRect.minX() / fWidth,
                   pixelRect.minY() / fHeight,
                   pixelRect.maxX() / fWidth,
                   pixelRect.maxY() / fHeight);
}

bool GlyphPacker::IsFull() const { return m_isFull; }

GlyphIndex::GlyphIndex(m2::PointU size, RefPointer<GlyphManager> mng)
  : m_packer(size)
  , m_mng(mng)
{
}

RefPointer<Texture::ResourceInfo> GlyphIndex::MapResource(GlyphKey const & key)
{
  strings::UniChar uniChar = key.GetUnicodePoint();
  auto it = m_index.find(uniChar);
  if (it != m_index.end())
    return MakeStackRefPointer<Texture::ResourceInfo>(&it->second);

  GlyphManager::Glyph glyph = m_mng->GetGlyph(uniChar);
  m2::RectU r;
  if (!m_packer.PackGlyph(glyph.m_image.m_width, glyph.m_image.m_height, r))
  {
    glyph.m_image.Destroy();
    return RefPointer<GlyphInfo>();
  }

  m_pendingNodes.emplace_back(r, glyph);

  auto res = m_index.emplace(uniChar, GlyphInfo(m_packer.MapTextureCoords(r), glyph.m_metrics));
  ASSERT(res.second, ());
  return MakeStackRefPointer<GlyphInfo>(&res.first->second);
}

void GlyphIndex::UploadResources(RefPointer<Texture> texture)
{
  if (m_pendingNodes.empty())
    return;

  buffer_vector<size_t, 3> ranges;
  buffer_vector<uint32_t, 2> maxHeights;
  ranges.push_back(0);
  uint32_t maxHeight = m_pendingNodes[0].first.SizeY();
  for (size_t i = 1; i < m_pendingNodes.size(); ++i)
  {
    TPendingNode const & prevNode = m_pendingNodes[i - 1];
    maxHeight = max(maxHeight, prevNode.first.SizeY());
    TPendingNode const & currentNode = m_pendingNodes[i];
    if (ranges.size() < 2 && prevNode.first.minY() < currentNode.first.minY())
    {
      ranges.push_back(i);
      maxHeights.push_back(maxHeight);
      maxHeight = currentNode.first.SizeY();
    }
  }
  maxHeights.push_back(maxHeight);
  ranges.push_back(m_pendingNodes.size());

  ASSERT(maxHeights.size() < 3, ());
  ASSERT(ranges.size() < 4, ());

  for (size_t i = 1; i < ranges.size(); ++i)
  {
    size_t startIndex = ranges[i - 1];
    size_t endIndex = ranges[i];
    uint32_t height = maxHeights[i - 1];
    uint32_t width = m_pendingNodes[endIndex - 1].first.maxX() - m_pendingNodes[startIndex].first.minX();
    uint32_t byteCount = my::NextPowOf2(height * width);
    m2::PointU zeroPoint = m_pendingNodes[startIndex].first.LeftBottom();

    SharedBufferManager::shared_buffer_ptr_t buffer = SharedBufferManager::instance().reserveSharedBuffer(byteCount);
    uint8_t * dstMemory = SharedBufferManager::GetRawPointer(buffer);
    view_t dstView = interleaved_view(width, height, (pixel_t *)dstMemory, width);
    for (size_t node = startIndex; node < endIndex; ++node)
    {
      GlyphManager::Glyph & glyph = m_pendingNodes[node].second;
      m2::RectU rect = m_pendingNodes[node].first;
      if (rect.SizeX() == 0 || rect.SizeY() == 0)
        continue;

      rect.Offset(-zeroPoint);

      uint32_t w = rect.SizeX();
      uint32_t h = rect.SizeY();

      ASSERT_EQUAL(glyph.m_image.m_width, w, ());
      ASSERT_EQUAL(glyph.m_image.m_height, h, ());

      view_t dstSubView = subimage_view(dstView, rect.minX(), rect.minY(), w, h);
      uint8_t * srcMemory = SharedBufferManager::GetRawPointer(glyph.m_image.m_data);
      const_view_t srcView = interleaved_view(w, h, (const_pixel_t *)srcMemory, w);

      copy_pixels(srcView, dstSubView);
      glyph.m_image.Destroy();
    }

    texture->UploadData(zeroPoint.x, zeroPoint.y, width, height, dp::ALPHA, MakeStackRefPointer<void>(dstMemory));
    SharedBufferManager::instance().freeSharedBuffer(byteCount, buffer);
  }

  m_pendingNodes.clear();
}

} // namespace dp
