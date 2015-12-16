#include "drape/font_texture.hpp"
#include "drape/pointers.hpp"
#include "3party/stb_image/stb_image.h"

#include "platform/platform.hpp"
#include "coding/reader.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"
#include "base/stl_add.hpp"

#include "std/chrono.hpp"
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

  // Half-pixel offset to eliminate arfefacts on fetching from texture.
  float offset = 0.0f;
  if (pixelRect.SizeX() != 0 && pixelRect.SizeY() != 0)
    offset = 0.5f;

  return m2::RectF((pixelRect.minX() + offset) / fWidth,
                   (pixelRect.minY() + offset) / fHeight,
                   (pixelRect.maxX() - offset) / fWidth,
                   (pixelRect.maxY() - offset) / fHeight);
}

bool GlyphPacker::IsFull() const { return m_isFull; }

GlyphGenerator::GlyphGenerator(ref_ptr<GlyphManager> mng, TCompletionHandler const & completionHandler)
  : m_mng(mng)
  , m_completionHandler(completionHandler)
  , m_isRunning(true)
  , m_isSuspended(false)
  , m_thread(&GlyphGenerator::Routine, this)
{
  ASSERT(m_completionHandler != nullptr, ());
}

GlyphGenerator::~GlyphGenerator()
{
  m_isRunning = false;
  m_condition.notify_one();
  m_thread.join();
  m_completionHandler = nullptr;

  for (GlyphGenerationData & data : m_queue)
    data.m_glyph.m_image.Destroy();
  m_queue.clear();
}

void GlyphGenerator::WaitForGlyph(list<GlyphGenerationData> & queue)
{
  unique_lock<mutex> lock(m_queueLock);
  m_isSuspended = true;
  m_condition.wait(lock, [this] { return !m_queue.empty() || !m_isRunning; });
  m_isSuspended = false;
  queue.swap(m_queue);
}

bool GlyphGenerator::IsSuspended() const
{
  lock_guard<mutex> lock(m_queueLock);
  return m_isSuspended;
}

void GlyphGenerator::Routine(GlyphGenerator * generator)
{
  ASSERT(generator != nullptr, ());
  while (generator->m_isRunning)
  {
    list<GlyphGenerationData> queue;
    generator->WaitForGlyph(queue);

    // generate glyphs
    for (GlyphGenerationData & data : queue)
    {
      GlyphManager::Glyph glyph = generator->m_mng->GenerateGlyph(data.m_glyph);
      data.m_glyph.m_image.Destroy();
      generator->m_completionHandler(data.m_rect, glyph);
    }
  }
}

void GlyphGenerator::GenerateGlyph(m2::RectU const & rect, GlyphManager::Glyph const & glyph)
{
  lock_guard<mutex> lock(m_queueLock);
  m_queue.emplace_back(rect, glyph);
  m_condition.notify_one();
}

GlyphIndex::GlyphIndex(m2::PointU size, ref_ptr<GlyphManager> mng)
  : m_packer(size)
  , m_mng(mng)
  , m_generator(new GlyphGenerator(mng, bind(&GlyphIndex::OnGlyphGenerationCompletion, this, _1, _2)))
{
  // Cache invalid glyph.
  GlyphKey const key = GlyphKey(m_mng->GetInvalidGlyph().m_code);
  bool newResource = false;
  MapResource(key, newResource);
}

GlyphIndex::~GlyphIndex()
{
  m_generator.reset();
  {
    threads::MutexGuard g(m_lock);
    for_each(m_pendingNodes.begin(), m_pendingNodes.end(), [](TPendingNode & node)
    {
      node.second.m_image.Destroy();
    });
    m_pendingNodes.clear();
  }
}

ref_ptr<Texture::ResourceInfo> GlyphIndex::MapResource(GlyphKey const & key, bool & newResource)
{
  newResource = false;
  strings::UniChar uniChar = key.GetUnicodePoint();
  auto it = m_index.find(uniChar);
  if (it != m_index.end())
    return make_ref(&it->second);

  newResource = true;

  GlyphManager::Glyph glyph = m_mng->GetGlyph(uniChar);
  m2::RectU r;
  if (!m_packer.PackGlyph(glyph.m_image.m_width, glyph.m_image.m_height, r))
  {
    glyph.m_image.Destroy();
    LOG(LWARNING, ("Glyphs packer could not pack a glyph", uniChar));

    auto invalidGlyph = m_index.find(m_mng->GetInvalidGlyph().m_code);
    if (invalidGlyph != m_index.end())
      return make_ref(&invalidGlyph->second);

    return nullptr;
  }

  m_generator->GenerateGlyph(r, glyph);

  auto res = m_index.emplace(uniChar, GlyphInfo(m_packer.MapTextureCoords(r), glyph.m_metrics));
  ASSERT(res.second, ());
  return make_ref(&res.first->second);
}

bool GlyphIndex::HasAsyncRoutines() const
{
  return !m_generator->IsSuspended();
}

size_t GlyphIndex::GetPendingNodesCount()
{
  threads::MutexGuard g(m_lock);
  return m_pendingNodes.size();
}

void GlyphIndex::OnGlyphGenerationCompletion(m2::RectU const & rect, GlyphManager::Glyph const & glyph)
{
  threads::MutexGuard g(m_lock);
  m_pendingNodes.emplace_back(rect, glyph);
}

void GlyphIndex::UploadResources(ref_ptr<Texture> texture)
{
  if (m_pendingNodes.empty())
    return;

  TPendingNodes pendingNodes;
  {
    threads::MutexGuard g(m_lock);
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

  buffer_vector<size_t, 3> ranges;
  buffer_vector<uint32_t, 2> maxHeights;
  ranges.push_back(0);
  uint32_t maxHeight = pendingNodes[0].first.SizeY();
  for (size_t i = 1; i < pendingNodes.size(); ++i)
  {
    TPendingNode const & prevNode = pendingNodes[i - 1];
    TPendingNode const & currentNode = pendingNodes[i];
    if (ranges.size() < 2 && prevNode.first.minY() < currentNode.first.minY())
    {
      ranges.push_back(i);
      maxHeights.push_back(maxHeight);
      maxHeight = currentNode.first.SizeY();
    }

    maxHeight = max(maxHeight, currentNode.first.SizeY());
  }
  maxHeights.push_back(maxHeight);
  ranges.push_back(pendingNodes.size());

  ASSERT(maxHeights.size() < 3, ());
  ASSERT(ranges.size() < 4, ());

  for (size_t i = 1; i < ranges.size(); ++i)
  {
    size_t startIndex = ranges[i - 1];
    size_t endIndex = ranges[i];
    uint32_t height = maxHeights[i - 1];
    uint32_t width = pendingNodes[endIndex - 1].first.maxX() - pendingNodes[startIndex].first.minX();
    uint32_t byteCount = my::NextPowOf2(height * width);

    if (byteCount == 0)
      continue;

    m2::PointU zeroPoint = pendingNodes[startIndex].first.LeftBottom();

    SharedBufferManager::shared_buffer_ptr_t buffer = SharedBufferManager::instance().reserveSharedBuffer(byteCount);
    uint8_t * dstMemory = SharedBufferManager::GetRawPointer(buffer);
    view_t dstView = interleaved_view(width, height, (pixel_t *)dstMemory, width);
    for (size_t node = startIndex; node < endIndex; ++node)
    {
      GlyphManager::Glyph & glyph = pendingNodes[node].second;
      m2::RectU rect = pendingNodes[node].first;
      if (rect.SizeX() == 0 || rect.SizeY() == 0)
        continue;

      rect.Offset(-zeroPoint);

      uint32_t w = rect.SizeX();
      uint32_t h = rect.SizeY();

      ASSERT_EQUAL(glyph.m_image.m_width, w, ());
      ASSERT_EQUAL(glyph.m_image.m_height, h, ());
      ASSERT_GREATER_OR_EQUAL(height, h, ());

      view_t dstSubView = subimage_view(dstView, rect.minX(), rect.minY(), w, h);
      uint8_t * srcMemory = SharedBufferManager::GetRawPointer(glyph.m_image.m_data);
      const_view_t srcView = interleaved_view(w, h, (const_pixel_t *)srcMemory, w);

      copy_pixels(srcView, dstSubView);
      glyph.m_image.Destroy();
    }

    texture->UploadData(zeroPoint.x, zeroPoint.y, width, height, make_ref(dstMemory));
    SharedBufferManager::instance().freeSharedBuffer(byteCount, buffer);
  }
}

} // namespace dp
