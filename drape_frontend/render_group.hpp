#pragma once

#include "drape_frontend/tile_utils.hpp"

#include "drape/pointers.hpp"
#include "drape/glstate.hpp"
#include "drape/render_bucket.hpp"

#include "std/vector.hpp"
#include "std/set.hpp"

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
  void CollectOverlay(dp::RefPointer<dp::OverlayTree> tree);
  void Render(ScreenBase const & screen);

  void PrepareForAdd(size_t countForAdd);
  void AddBucket(dp::TransferPointer<dp::RenderBucket> bucket);

  bool IsEmpty() const { return m_renderBuckets.empty(); }
  void DeleteLater() const { m_pendingOnDelete = true; }
  bool IsPendingOnDelete() const { return m_pendingOnDelete; }

  bool IsLess(RenderGroup const & other) const;

private:
  vector<dp::MasterPointer<dp::RenderBucket> > m_renderBuckets;

  mutable bool m_pendingOnDelete;
};

class RenderGroupComparator
{
public:
  RenderGroupComparator(TTilesCollection const & activeTiles);

  void ResetInternalState();

  bool operator()(RenderGroup const * l, RenderGroup const * r);

private:
  TTilesCollection const & m_activeTiles;
  bool m_needGroupMergeOperation;
  bool m_needBucketsMergeOperation;

  bool NeedToDelete(TileKey const & key);
};

class UserMarkRenderGroup : public BaseRenderGroup
{
  typedef BaseRenderGroup TBase;

public:
  UserMarkRenderGroup(dp::GLState const & state, TileKey const & tileKey);
  ~UserMarkRenderGroup();

  void SetIsVisible(bool isVisible);
  bool IsVisible();

  void Clear();
  void SetRenderBucket(dp::GLState const & state, dp::TransferPointer<dp::RenderBucket> bucket);
  void Render(ScreenBase const & screen);

private:
  bool m_isVisible;
  dp::MasterPointer<dp::RenderBucket> m_renderBucket;
};

} // namespace df
