#pragma once

#include "drape/drape_routine.hpp"
#include "drape/glyph_manager.hpp"
#include "drape/pointers.hpp"

#include "geometry/rect2d.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

namespace dp
{
class GlyphGenerator
{
public:
  struct GlyphGenerationData
  {
    m2::RectU m_rect;
    GlyphManager::Glyph m_glyph;

    GlyphGenerationData() = default;
    GlyphGenerationData(m2::RectU const & rect, GlyphManager::Glyph const & glyph)
      : m_rect(rect), m_glyph(glyph)
    {}

    void DestroyGlyph()
    {
      m_glyph.m_image.Destroy();
    }
  };

  using GlyphGenerationDataArray = std::vector<GlyphGenerationData>;

  class Listener
  {
  public:
    virtual ~Listener() = default;
    virtual void OnCompleteGlyphGeneration(GlyphGenerationDataArray && glyphs) = 0;
  };

  class GenerateGlyphTask
  {
  public:
    explicit GenerateGlyphTask(GlyphGenerationDataArray && glyphs)
      : m_glyphs(std::move(glyphs))
      , m_isCancelled(false)
    {}
    void Run(uint32_t sdfScale);
    void Cancel() { m_isCancelled = true; }

    GlyphGenerationDataArray && StealGeneratedGlyphs() { return std::move(m_generatedGlyphs); }
    bool IsCancelled() const { return m_isCancelled; }
    void DestroyAllGlyphs();

  private:
    GlyphGenerationDataArray m_glyphs;
    GlyphGenerationDataArray m_generatedGlyphs;
    std::atomic<bool> m_isCancelled;
  };

  explicit GlyphGenerator(uint32_t sdfScale);
  ~GlyphGenerator();

  void RegisterListener(ref_ptr<Listener> listener);
  void UnregisterListener(ref_ptr<Listener> listener);

  void GenerateGlyph(ref_ptr<Listener> listener, m2::RectU const & rect, GlyphManager::Glyph & glyph);
  void GenerateGlyph(ref_ptr<Listener> listener, GlyphGenerationData && data);
  void GenerateGlyphs(ref_ptr<Listener> listener, GlyphGenerationDataArray && generationData);
  bool IsSuspended() const;

private:
  void OnTaskFinished(ref_ptr<Listener> listener, std::shared_ptr<GenerateGlyphTask> const & task);

  uint32_t m_sdfScale;
  std::set<ref_ptr<Listener>> m_listeners;
  ActiveTasks<GenerateGlyphTask> m_activeTasks;

  GlyphGenerationDataArray m_queue;
  size_t m_glyphsCounter = 0;
  mutable std::mutex m_mutex;
};
}  // namespace dp
