#pragma once

#include "drape/pointers.hpp"

#include "indexer/feature_decl.hpp"

#include "geometry/rect2d.hpp"

#include <vector>

class ScreenBase;
namespace dp
{
class OverlayHandle;
}

namespace df
{
class RenderGroup;

// Search symbols cover regular textured symbols only at their current screen position.
class SearchMarkOverlayFilter
{
public:
  void Collect(std::vector<drape_ptr<RenderGroup>> const & searchGroups, ScreenBase const & screen);
  void HideOverlappingSymbols(RenderGroup & group, ScreenBase const & screen);
  void RestoreHiddenSymbols();

private:
  struct Entry
  {
    FeatureID m_featureId;
    m2::RectD m_rect;
  };

  std::vector<Entry> m_entries;
  std::vector<ref_ptr<dp::OverlayHandle>> m_hiddenSymbols;
};
}  // namespace df
