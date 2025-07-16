#pragma once

#include "drape_frontend/animation/opacity_animation.hpp"
#include "drape_frontend/animation/value_mapping.hpp"
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

/// @todo Actually, there is no need in this polymorphic interface.
class BaseRenderGroup
{
public:
  BaseRenderGroup(dp::RenderState const & state, TileKey const & tileKey) : m_state(state), m_tileKey(tileKey) {}

  virtual ~BaseRenderGroup() = default;

  dp::RenderState const & GetState() const { return m_state; }
  TileKey const & GetTileKey() const { return m_tileKey; }

  virtual void UpdateAnimation();
  virtual void Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, ScreenBase const & screen,
                      FrameValues const & frameValues, ref_ptr<DebugRectRenderer> debugRectRenderer) = 0;

protected:
  dp::RenderState m_state;
  gpu::MapProgramParams m_params;

private:
  TileKey m_tileKey;
};

class RenderGroup : public BaseRenderGroup
{
  using TBase = BaseRenderGroup;

public:
  RenderGroup(dp::RenderState const & state, TileKey const & tileKey);
  ~RenderGroup() override;

  void Update(ScreenBase const & modelView);
  void CollectOverlay(ref_ptr<dp::OverlayTree> tree);
  bool HasOverlayHandles() const;
  void RemoveOverlay(ref_ptr<dp::OverlayTree> tree);
  void SetOverlayVisibility(bool isVisible);
  void Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, ScreenBase const & screen,
              FrameValues const & frameValues, ref_ptr<DebugRectRenderer> debugRectRenderer) override;

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
  std::vector<drape_ptr<dp::RenderBucket>> m_renderBuckets;
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

class UserMarkRenderGroup : public RenderGroup
{
  using TBase = RenderGroup;

public:
  UserMarkRenderGroup(dp::RenderState const & state, TileKey const & tileKey);

  void UpdateAnimation() override;

  bool IsUserPoint() const;

private:
  std::unique_ptr<OpacityAnimation> m_animation;
  ValueMapping<float> m_mapping;
};
}  // namespace df
