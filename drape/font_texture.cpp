#include "drape/font_texture.hpp"

#include "drape/pointers.hpp"

#include "platform/platform.hpp"
#include "coding/reader.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"
#include "base/stl_add.hpp"

#include <functional>

using namespace std::placeholders;

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
  m_yStep = max(height, m_yStep);
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
    step = max(height, step);
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

void GlyphGenerator::GenerateGlyphTask::Run(uint32_t sdfScale)
{
  m_generatedGlyphs.reserve(m_glyphs.size());
  for (auto & data : m_glyphs)
  {
    if (m_isCancelled)
      return;

    auto const g = GlyphManager::GenerateGlyph(data.m_glyph, sdfScale);
    data.m_glyph.m_image.Destroy();
    m_generatedGlyphs.emplace_back(GlyphGenerator::GlyphGenerationData{data.m_rect, g});
  }
}

void GlyphGenerator::GenerateGlyphTask::DestroyAllGlyphs()
{
  for (auto & data : m_glyphs)
    data.m_glyph.m_image.Destroy();

  for (auto & data : m_generatedGlyphs)
    data.m_glyph.m_image.Destroy();
}

GlyphGenerator::GlyphGenerator(uint32_t sdfScale,
                               CompletionHandler const & completionHandler)
  : m_sdfScale(sdfScale)
  , m_completionHandler(completionHandler)
{
  ASSERT(m_completionHandler != nullptr, ());
}

GlyphGenerator::~GlyphGenerator()
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_completionHandler = nullptr;
  }

  // Here we have to wait for active tasks completion,
  // because they capture 'this' pointer.
  m_activeTasks.FinishAll();

  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto & data : m_queue)
    data.m_glyph.m_image.Destroy();
  m_queue.clear();
}

bool GlyphGenerator::IsSuspended() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_glyphsCounter == 0;
}

void GlyphGenerator::GenerateGlyph(m2::RectU const & rect, GlyphManager::Glyph & glyph)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_completionHandler)
  {
    glyph.m_image.Destroy();
    return;
  }

  std::list<GlyphGenerationData> queue;
  m_queue.emplace_back(rect, glyph);
  std::swap(m_queue, queue);
  m_glyphsCounter += queue.size();

  // Generate glyphs on the separate thread.
  auto generateTask = std::make_shared<GenerateGlyphTask>(std::move(queue));
  auto result = DrapeRoutine::Run([this, generateTask]() mutable
  {
    generateTask->Run(m_sdfScale);
    OnTaskFinished(generateTask);
  });

  if (result)
    m_activeTasks.Add(generateTask, result);
  else
    generateTask->DestroyAllGlyphs();
}

void GlyphGenerator::OnTaskFinished(std::shared_ptr<GenerateGlyphTask> const & task)
{
  if (task->IsCancelled())
  {
    task->DestroyAllGlyphs();
    return;
  }

  std::vector<GlyphGenerationData> glyphs = task->StealGeneratedGlyphs();

  std::lock_guard<std::mutex> lock(m_mutex);
  ASSERT_GREATER_OR_EQUAL(m_glyphsCounter, glyphs.size(), ());
  m_glyphsCounter -= glyphs.size();
  m_completionHandler(std::move(glyphs));

  m_activeTasks.Remove(task);
}

GlyphIndex::GlyphIndex(m2::PointU size, ref_ptr<GlyphManager> mng)
  : m_packer(size)
  , m_mng(mng)
  , m_generator(my::make_unique<GlyphGenerator>(mng->GetSdfScale(),
                std::bind(&GlyphIndex::OnGlyphGenerationCompletion, this, _1)))
{
  // Cache invalid glyph.
  auto const key = GlyphKey(m_mng->GetInvalidGlyph(GlyphManager::kDynamicGlyphSize).m_code,
                            GlyphManager::kDynamicGlyphSize);
  bool newResource = false;
  MapResource(key, newResource);
}

GlyphIndex::~GlyphIndex()
{
  m_generator.reset();

  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto & node : m_pendingNodes)
    node.second.m_image.Destroy();
  m_pendingNodes.clear();
}

ref_ptr<Texture::ResourceInfo> GlyphIndex::MapResource(GlyphKey const & key, bool & newResource)
{
  newResource = false;
  auto it = m_index.find(key);
  if (it != m_index.end())
    return make_ref(&it->second);

  newResource = true;

  GlyphManager::Glyph glyph = m_mng->GetGlyph(key.GetUnicodePoint(), key.GetFixedSize());
  m2::RectU r;
  if (!m_packer.PackGlyph(glyph.m_image.m_width, glyph.m_image.m_height, r))
  {
    glyph.m_image.Destroy();
    LOG(LWARNING, ("Glyphs packer could not pack a glyph", key.GetUnicodePoint(),
                   "w =", glyph.m_image.m_width, "h =", glyph.m_image.m_height,
                   "packerSize =", m_packer.GetSize()));

    auto invalidGlyph = m_index.find(GlyphKey(m_mng->GetInvalidGlyph(key.GetFixedSize()).m_code,
                                              key.GetFixedSize()));
    if (invalidGlyph != m_index.end())
      return make_ref(&invalidGlyph->second);

    return nullptr;
  }

  m_generator->GenerateGlyph(r, glyph);

  auto res = m_index.emplace(key, GlyphInfo(m_packer.MapTextureCoords(r), glyph.m_metrics));
  ASSERT(res.second, ());
  return make_ref(&res.first->second);
}

bool GlyphIndex::CanBeGlyphPacked(uint32_t glyphsCount) const
{
  if (glyphsCount == 0)
    return true;

  if (m_packer.IsFull())
    return false;

  float const kGlyphScalar = 1.5f;
  auto const baseSize = static_cast<uint32_t>(m_mng->GetBaseGlyphHeight() * kGlyphScalar);
  return m_packer.CanBePacked(glyphsCount, baseSize, baseSize);
}

bool GlyphIndex::HasAsyncRoutines() const
{
  return !m_generator->IsSuspended();
}

size_t GlyphIndex::GetPendingNodesCount()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_pendingNodes.size();
}

void GlyphIndex::OnGlyphGenerationCompletion(std::vector<GlyphGenerator::GlyphGenerationData> && glyphs)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto & g : glyphs)
    m_pendingNodes.emplace_back(g.m_rect, g.m_glyph);
}

void GlyphIndex::UploadResources(ref_ptr<Texture> texture)
{
  PendingNodes pendingNodes;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
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

uint32_t GlyphIndex::GetAbsentGlyphsCount(strings::UniString const & text, int fixedHeight) const
{
  uint32_t count = 0;
  for (strings::UniChar const & c : text)
  {
    if (m_index.find(GlyphKey(c, fixedHeight)) == m_index.end())
      count++;
  }
  return count;
}
}  // namespace dp
