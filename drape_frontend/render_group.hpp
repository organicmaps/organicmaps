#pragma once

#include "tile_key.hpp"

#include "../drape/pointers.hpp"
#include "../drape/glstate.hpp"
#include "../drape/render_bucket.hpp"

#include "../std/vector.hpp"
#include "../std/set.hpp"

class OverlayTree;

namespace df
{
  class RenderGroup
  {
  public:
    RenderGroup(GLState const & state, TileKey const & tileKey);
    ~RenderGroup();

    void CollectOverlay(RefPointer<OverlayTree> tree);
    void Render();

    void PrepareForAdd(size_t countForAdd);
    void AddBucket(TransferPointer<RenderBucket> bucket);

    GLState const & GetState() const { return m_state; }
    TileKey const & GetTileKey() const { return m_tileKey; }

    bool IsEmpty() const { return m_renderBuckets.empty(); }
    void DeleteLater() const { m_pendingOnDelete = true; }
    bool IsPendingOnDelete() const { return m_pendingOnDelete; }

    bool IsLess(RenderGroup const & other) const;

  private:
    GLState m_state;
    TileKey m_tileKey;
    vector<MasterPointer<RenderBucket> > m_renderBuckets;

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
}
