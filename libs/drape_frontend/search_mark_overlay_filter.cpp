#include "drape_frontend/search_mark_overlay_filter.hpp"

#include "drape_frontend/render_group.hpp"

#include "shaders/programs.hpp"

#include "drape/overlay_handle.hpp"

#include "geometry/screenbase.hpp"

#include "base/assert.hpp"

#include <algorithm>

namespace df
{
void SearchMarkOverlayFilter::Collect(std::vector<drape_ptr<RenderGroup>> const & searchGroups,
                                      ScreenBase const & screen)
{
  ASSERT(m_hiddenSymbols.empty(), ());
  m_entries.clear();
  for (auto const & group : searchGroups)
  {
    auto const program = group->GetState().GetProgram<gpu::Program>();
    if (program == gpu::Program::Text || program == gpu::Program::TextOutlined)
      continue;

    group->ForEachOverlay([&](ref_ptr<dp::OverlayHandle> handle)
    {
      auto const & id = handle->GetOverlayID().m_featureId;
      if (handle->IsVisible() && id.IsValid())
        m_entries.push_back({id, handle->GetPixelRect(screen, screen.isPerspective())});
    });
  }
  std::sort(m_entries.begin(), m_entries.end(),
            [](Entry const & l, Entry const & r) { return l.m_featureId < r.m_featureId; });
}

void SearchMarkOverlayFilter::HideOverlappingSymbols(RenderGroup & group, ScreenBase const & screen)
{
  ASSERT(m_hiddenSymbols.empty(), ());
  if (m_entries.empty())
    return;

  auto const program = group.GetState().GetProgram<gpu::Program>();
  if (program != gpu::Program::Texturing && program != gpu::Program::MaskedTexturing)
    return;

  group.ForEachOverlay([&](ref_ptr<dp::OverlayHandle> handle)
  {
    if (!handle->IsVisible())
      return;

    auto const & id = handle->GetOverlayID().m_featureId;
    if (!id.IsValid())
      return;

    auto it =
        std::lower_bound(m_entries.begin(), m_entries.end(), id, [](Entry const & entry, FeatureID const & featureId)
    { return entry.m_featureId < featureId; });
    if (it == m_entries.end() || it->m_featureId != id)
      return;

    auto const rect = handle->GetPixelRect(screen, screen.isPerspective());
    for (; it != m_entries.end() && it->m_featureId == id; ++it)
    {
      if (rect.IsIntersect(it->m_rect))
      {
        m_hiddenSymbols.push_back(handle);
        handle->SetIsVisible(false);
        break;
      }
    }
  });
}

void SearchMarkOverlayFilter::RestoreHiddenSymbols()
{
  for (auto const & handle : m_hiddenSymbols)
    handle->SetIsVisible(true);
  m_hiddenSymbols.clear();
}
}  // namespace df
