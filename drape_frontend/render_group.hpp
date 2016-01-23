#pragma once

#include "drape_frontend/animation/opacity_animation.hpp"
#include "drape_frontend/animation/value_mapping.hpp"
#include "drape_frontend/tile_utils.hpp"

#include "drape/pointers.hpp"
#include "drape/glstate.hpp"
#include "drape/render_bucket.hpp"

#include "std/vector.hpp"
#include "std/set.hpp"
#include "std/unique_ptr.hpp"

class ScreenBase;
namespace dp { class OverlayTree; }

namespace df
{

class BaseRenderGroup
{
public:
  BaseRenderGroup(dp::GLState const & state, TileKey const & tileKey)
    : m_state(state)
    , m_tileKey(tileKey) {}

  void SetRenderParams(ref_ptr<dp::GpuProgram> shader, ref_ptr<dp::GpuProgram> shader3d,
                       ref_ptr<dp::UniformValuesStorage> generalUniforms);

  dp::GLState const & GetState() const { return m_state; }
  TileKey const & GetTileKey() const { return m_tileKey; }
  dp::UniformValuesStorage const & GetUniforms() const { return m_uniforms; }
  bool IsOverlay() const;

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
  typedef BaseRenderGroup TBase;
  friend class BatchMergeHelper;
public:
  RenderGroup(dp::GLState const & state, TileKey const & tileKey);
  ~RenderGroup();

  void Update(ScreenBase const & modelView);
  void CollectOverlay(ref_ptr<dp::OverlayTree> tree);
  void Render(ScreenBase const & screen) override;

  void AddBucket(drape_ptr<dp::RenderBucket> && bucket);

  bool IsEmpty() const { return m_renderBuckets.empty(); }
  void DeleteLater() const { m_pendingOnDelete = true; }
  bool IsPendingOnDelete() const { return m_pendingOnDelete; }
  bool CanBeDeleted() const { return IsPendingOnDelete() && !IsAnimating(); }

  bool IsLess(RenderGroup const & other) const;

  void UpdateAnimation() override;
  double GetOpacity() const;
  bool IsAnimating() const;

  void Appear();
  void Disappear();

private:
  vector<drape_ptr<dp::RenderBucket> > m_renderBuckets;
  unique_ptr<OpacityAnimation> m_disappearAnimation;
  unique_ptr<OpacityAnimation> m_appearAnimation;

  mutable bool m_pendingOnDelete;

private:
  friend string DebugPrint(RenderGroup const & group);
};

class RenderGroupComparator
{
public:
  bool operator()(drape_ptr<RenderGroup> const & l, drape_ptr<RenderGroup> const & r);
  bool m_pendingOnDeleteFound = false;
};

class UserMarkRenderGroup : public BaseRenderGroup
{
  typedef BaseRenderGroup TBase;

public:
  UserMarkRenderGroup(dp::GLState const & state, TileKey const & tileKey,
                      drape_ptr<dp::RenderBucket> && bucket);
  ~UserMarkRenderGroup();

  void UpdateAnimation() override;
  void Render(ScreenBase const & screen) override;

private:
  drape_ptr<dp::RenderBucket> m_renderBucket;
  unique_ptr<OpacityAnimation> m_animation;
  ValueMapping<float> m_mapping;
};

} // namespace df
