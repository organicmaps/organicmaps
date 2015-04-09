#pragma once

#include "drape_frontend/tile_key.hpp"

#include "drape/pointers.hpp"
#include "drape/glstate.hpp"
#include "drape/render_bucket.hpp"

#include "std/vector.hpp"
#include "std/set.hpp"

class ScreenBase;
namespace dp { class OverlayTree; }

namespace df
{

class RenderGroup
{
public:
  RenderGroup(dp::GLState const & state, TileKey const & tileKey);
  ~RenderGroup();

  void Update(ScreenBase const & modelView);
  void CollectOverlay(dp::RefPointer<dp::OverlayTree> tree);
  void Render(ScreenBase const & screen);

  void PrepareForAdd(size_t countForAdd);
  void AddBucket(dp::TransferPointer<dp::RenderBucket> bucket);

  dp::GLState const & GetState() const { return m_state; }
  TileKey const & GetTileKey() const { return m_tileKey; }

  bool IsEmpty() const { return m_renderBuckets.empty(); }
  void DeleteLater() const { m_pendingOnDelete = true; }
  bool IsPendingOnDelete() const { return m_pendingOnDelete; }

  bool IsLess(RenderGroup const & other) const;

private:
  dp::GLState m_state;
  TileKey m_tileKey;
  vector<dp::MasterPointer<dp::RenderBucket> > m_renderBuckets;

  mutable bool m_pendingOnDelete;
};

class RenderBucketComparator
{
public:
  RenderBucketComparator(set<TileKey> const & activeTiles);

  void ResetInternalState();

  bool operator()(RenderGroup const * l, RenderGroup const * r);

private:
  set<TileKey> const & m_activeTiles;
  bool m_needGroupMergeOperation;
  bool m_needBucketsMergeOperation;
};

} // namespace df
