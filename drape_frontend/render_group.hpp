#pragma once

#include "drape_frontend/animation/opacity_animation.hpp"
#include "drape_frontend/animation/value_mapping.hpp"
#include "drape_frontend/render_state.hpp"
#include "drape_frontend/tile_utils.hpp"

#include "drape/pointers.hpp"
#include "drape/render_bucket.hpp"

#include <memory>
#include <vector>
#include <string>

class ScreenBase;
namespace dp { class OverlayTree; }

namespace df
{

class BaseRenderGroup
{
public:
  BaseRenderGroup(dp::GLState const & state, TileKey const & tileKey)
    : m_state(state)
    , m_tileKey(tileKey)
  {}

  virtual ~BaseRenderGroup() {}

  void SetRenderParams(ref_ptr<dp::GpuProgram> shader, ref_ptr<dp::GpuProgram> shader3d,
                       ref_ptr<dp::UniformValuesStorage> generalUniforms);

  dp::GLState const & GetState() const { return m_state; }
  TileKey const & GetTileKey() const { return m_tileKey; }
  dp::UniformValuesStorage const & GetUniforms() const { return m_uniforms; }

  virtual void UpdateAnimation();
  virtual void Render(ScreenBase const & screen);

protected:
  dp::GLState m_state;
  ref_ptr<dp::GpuProgram> m_shader;
  ref_ptr<dp::GpuProgram> m_shader3d;
  dp::UniformValuesStorage m_uniforms;
  ref_ptr<dp::UniformValuesStorage> m_generalUniforms;

private:
  TileKey m_tileKey;
};

class RenderGroup : public BaseRenderGroup
{
  using TBase = BaseRenderGroup;
  friend class BatchMergeHelper;
public:
  RenderGroup(dp::GLState const & state, TileKey const & tileKey);
  ~RenderGroup() override;

  void Update(ScreenBase const & modelView);
  void CollectOverlay(ref_ptr<dp::OverlayTree> tree);
  bool HasOverlayHandles() const;
  void RemoveOverlay(ref_ptr<dp::OverlayTree> tree);
  void Render(ScreenBase const & screen) override;

  void AddBucket(drape_ptr<dp::RenderBucket> && bucket);

  bool IsEmpty() const { return m_renderBuckets.empty(); }

  void DeleteLater() const { m_pendingOnDelete = true; }
  bool IsPendingOnDelete() const { return m_pendingOnDelete; }
  bool CanBeDeleted() const { return m_canBeDeleted; }

  bool UpdateCanBeDeletedStatus(bool canBeDeleted, int currentZoom, ref_ptr<dp::OverlayTree> tree);

  bool IsOverlay() const;
  bool IsUserMark() const;

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
  UserMarkRenderGroup(dp::GLState const & state, TileKey const & tileKey);
  ~UserMarkRenderGroup() override {}

  void UpdateAnimation() override;

  bool IsUserPoint() const;

private:
  std::unique_ptr<OpacityAnimation> m_animation;
  ValueMapping<float> m_mapping;
};
}  // namespace df
