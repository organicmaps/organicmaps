#pragma once

#include "drape_frontend/animation/opacity_animation.hpp"
#include "drape_frontend/frame_values.hpp"
#include "drape_frontend/render_state_extension.hpp"
#include "drape_frontend/tile_utils.hpp"

#include "shaders/program_params.hpp"

#include "drape/pointers.hpp"
#include "drape/render_bucket.hpp"

#include <memory>
#include <string>
#include <vector>

class ScreenBase;
namespace dp
{
class OverlayTree;
}
namespace gpu
{
class ProgramManager;
}

namespace df
{
class DebugRectRenderer;

class RenderGroup
{
public:
  RenderGroup(dp::RenderState const & state, TileKey const & tileKey);

  dp::RenderState const & GetState() const { return m_state; }
  TileKey const & GetTileKey() const { return m_tileKey; }

  void Update(ScreenBase const & modelView);
  void CollectOverlay(ref_ptr<dp::OverlayTree> tree);
  bool HasOverlayHandles() const;
  void RemoveOverlay(ref_ptr<dp::OverlayTree> tree);
  void SetOverlayVisibility(bool isVisible);
  void Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, ScreenBase const & screen,
              FrameValues const & frameValues, ref_ptr<DebugRectRenderer> debugRectRenderer);

  void AddBucket(drape_ptr<dp::RenderBucket> && bucket);

  bool IsEmpty() const { return m_renderBuckets.empty(); }

  void DeleteLater() const { m_pendingOnDelete = true; }
  bool IsPendingOnDelete() const { return m_pendingOnDelete; }
  bool CanBeDeleted() const { return m_canBeDeleted; }

  bool UpdateCanBeDeletedStatus(bool canBeDeleted, int currentZoom, ref_ptr<dp::OverlayTree> tree);

  bool IsUserMark() const;

  // Warning! Has linear complexity on number of OverlayHandles in the render group.
  template <typename ToDo>
  void ForEachOverlay(ToDo const & todo)
  {
    if (CanBeDeleted())
      return;

    for (auto & renderBucket : m_renderBuckets)
      renderBucket->ForEachOverlay(todo);
  }

private:
  dp::RenderState m_state;
  gpu::MapProgramParams m_params;
  TileKey m_tileKey;
  std::vector<drape_ptr<dp::RenderBucket>> m_renderBuckets;
  std::unique_ptr<OpacityAnimation> m_bookmarkAnimation;
  mutable bool m_pendingOnDelete;
  mutable bool m_canBeDeleted;

private:
  friend std::string DebugPrint(RenderGroup const & group);
};

class RenderGroupComparator
{
public:
  bool operator()(drape_ptr<RenderGroup> const & l, drape_ptr<RenderGroup> const & r);
  bool m_pendingOnDeleteFound = false;
};

}  // namespace df
