#include "drape/glyph_generator.hpp"

#include <iterator>

namespace dp
{
GlyphGenerator::GlyphGenerator(uint32_t sdfScale)
  : m_sdfScale(sdfScale)
{}

GlyphGenerator::~GlyphGenerator()
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listeners.clear();
  }

  // Here we have to wait for active tasks completion,
  // because they capture 'this' pointer.
  FinishGeneration();
}

bool GlyphGenerator::IsSuspended() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_glyphsCounter == 0;
}

void GlyphGenerator::FinishGeneration()
{
  m_activeTasks.FinishAll();

  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto & data : m_queue)
    data.DestroyGlyph();

  m_glyphsCounter = 0;
}

void GlyphGenerator::RegisterListener(ref_ptr<GlyphGenerator::Listener> listener)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_listeners.insert(listener);
}

void GlyphGenerator::UnregisterListener(ref_ptr<GlyphGenerator::Listener> listener)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_listeners.erase(listener);
}

void GlyphGenerator::GenerateGlyph(ref_ptr<Listener> listener, m2::RectU const & rect,
                                   GlyphManager::Glyph & glyph)
{
  GenerateGlyph(listener, GlyphGenerationData(rect, glyph));
}

void GlyphGenerator::GenerateGlyph(ref_ptr<Listener> listener, GlyphGenerationData && data)
{
  GenerateGlyphs(listener, {std::move(data)});
}

void GlyphGenerator::GenerateGlyphs(ref_ptr<Listener> listener,
                                    GlyphGenerationDataArray && generationData)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_listeners.find(listener) == m_listeners.end())
  {
    for (auto & data : generationData)
      data.DestroyGlyph();
    return;
  }

  GlyphGenerationDataArray queue;
  std::move(generationData.begin(), generationData.end(), std::back_inserter(m_queue));
  std::swap(m_queue, queue);
  m_glyphsCounter += queue.size();

  // Generate glyphs on the separate thread.
  auto generateTask = std::make_shared<GenerateGlyphTask>(std::move(queue));
  auto result = DrapeRoutine::Run([this, listener, generateTask]() mutable
  {
    generateTask->Run(m_sdfScale);
    OnTaskFinished(listener, generateTask);
  });

  if (result)
    m_activeTasks.Add(std::move(generateTask), std::move(result));
  else
    generateTask->DestroyAllGlyphs();
}

void GlyphGenerator::OnTaskFinished(ref_ptr<Listener> listener,
                                    std::shared_ptr<GenerateGlyphTask> const & task)
{
  if (task->IsCancelled())
  {
    task->DestroyAllGlyphs();
    return;
  }

  auto glyphs = task->StealGeneratedGlyphs();

  std::lock_guard<std::mutex> lock(m_mutex);
  ASSERT_GREATER_OR_EQUAL(m_glyphsCounter, glyphs.size(), ());
  m_glyphsCounter -= glyphs.size();

  if (m_listeners.find(listener) != m_listeners.end())
  {
    listener->OnCompleteGlyphGeneration(std::move(glyphs));
  }
  else
  {
    for (auto & data : glyphs)
      data.DestroyGlyph();
  }

  m_activeTasks.Remove(task);
}

void GlyphGenerator::GenerateGlyphTask::Run(uint32_t sdfScale)
{
  m_generatedGlyphs.reserve(m_glyphs.size());
  for (auto & data : m_glyphs)
  {
    if (m_isCancelled)
      return;

    auto const g = GlyphManager::GenerateGlyph(data.m_glyph, sdfScale);
    data.DestroyGlyph();
    m_generatedGlyphs.emplace_back(GlyphGenerator::GlyphGenerationData{data.m_rect, g});
  }
}

void GlyphGenerator::GenerateGlyphTask::DestroyAllGlyphs()
{
  for (auto & data : m_glyphs)
    data.DestroyGlyph();

  for (auto & data : m_generatedGlyphs)
    data.DestroyGlyph();
}
}  // namespace dp
