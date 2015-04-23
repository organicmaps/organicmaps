#pragma once

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

  dp::GLState const & GetState() const { return m_state; }
  TileKey const & GetTileKey() const { return m_tileKey; }

protected:
  dp::GLState m_state;

private:
  TileKey m_tileKey;
};

class RenderGroup : public BaseRenderGroup
{
  typedef BaseRenderGroup TBase;
public:
  RenderGroup(dp::GLState const & state, TileKey const & tileKey);
  ~RenderGroup();

  void Update(ScreenBase const & modelView);
  void CollectOverlay(ref_ptr<dp::OverlayTree> tree);
  void Render(ScreenBase const & screen);

  void PrepareForAdd(size_t countForAdd);
  void AddBucket(drape_ptr<dp::RenderBucket> && bucket);

  bool IsEmpty() const { return m_renderBuckets.empty(); }
  void DeleteLater() const { m_pendingOnDelete = true; }
  bool IsPendingOnDelete() const { return m_pendingOnDelete; }

  bool IsLess(RenderGroup const & other) const;

private:
  vector<drape_ptr<dp::RenderBucket> > m_renderBuckets;

  mutable bool m_pendingOnDelete;

private:
  friend string DebugPrint(RenderGroup const & group);
};

class RenderGroupComparator
{
public:
  RenderGroupComparator();

  void ResetInternalState();

  bool operator()(drape_ptr<RenderGroup> const & l, drape_ptr<RenderGroup> const & r);

private:
  bool m_needGroupMergeOperation;
  bool m_needBucketsMergeOperation;
};

class UserMarkRenderGroup : public BaseRenderGroup
{
  typedef BaseRenderGroup TBase;

public:
  UserMarkRenderGroup(dp::GLState const & state, TileKey const & tileKey,
                      drape_ptr<dp::RenderBucket> && bucket);
  ~UserMarkRenderGroup();

  void Render(ScreenBase const & screen);

private:
  drape_ptr<dp::RenderBucket> m_renderBucket;
};

} // namespace df
