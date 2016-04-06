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

  for (size_t i = 0; i < pendingNodes.size(); ++i)
  {
    GlyphManager::Glyph & glyph = pendingNodes[i].second;
    m2::RectU const rect = pendingNodes[i].first;
    m2::PointU const zeroPoint = rect.LeftBottom();
    if (glyph.m_image.m_width == 0 || glyph.m_image.m_height == 0 || rect.SizeX() == 0 || rect.SizeY() == 0)
    {
      LOG(LWARNING, ("Glyph skipped", glyph.m_code));
      continue;
    }
    ASSERT_EQUAL(glyph.m_image.m_width, rect.SizeX(), ());
    ASSERT_EQUAL(glyph.m_image.m_height, rect.SizeY(), ());

    uint8_t * srcMemory = SharedBufferManager::GetRawPointer(glyph.m_image.m_data);
    texture->UploadData(zeroPoint.x, zeroPoint.y, rect.SizeX(), rect.SizeY(), make_ref(srcMemory));
    
    glyph.m_image.Destroy();
  }
}

} // namespace dp
