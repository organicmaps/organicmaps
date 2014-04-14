#include "render_group.hpp"

#include "../base/stl_add.hpp"

namespace df
{
  RenderGroup::RenderGroup(const GLState & state, const df::TileKey & tileKey)
    : m_state(state)
    , m_tileKey(tileKey)
    , m_pendingOnDelete(false)
  {
  }

  RenderGroup::~RenderGroup()
  {
    (void)GetRangeDeletor(m_renderBuckets, MasterPointerDeleter())();
  }

  void RenderGroup::PrepareForAdd(size_t countForAdd)
  {
    m_renderBuckets.reserve(m_renderBuckets.size() + countForAdd);
  }

  void RenderGroup::AddBucket(TransferPointer<RenderBucket> bucket)
  {
    m_renderBuckets.push_back(MasterPointer<RenderBucket>(bucket));
  }

  bool RenderGroup::IsLess(const RenderGroup & other) const
  {
    return m_state < other.m_state;
  }

  RenderBucketComparator::RenderBucketComparator(const set<TileKey> & activeTiles)
    : m_activeTiles(activeTiles)
    , m_needGroupMergeOperation(false)
    , m_needBucketsMergeOperation(false)
  {
  }

  void RenderBucketComparator::ResetInternalState()
  {
    m_needBucketsMergeOperation = false;
    m_needGroupMergeOperation = false;
  }

  bool RenderBucketComparator::operator()(const RenderGroup & l, const RenderGroup & r)
  {
    GLState const & lState = l.GetState();
    GLState const & rState = r.GetState();

    TileKey const & lKey = l.GetTileKey();
    TileKey const & rKey = r.GetTileKey();

    bool lCheckDeletion = !(l.IsEmpty() || l.IsPendingOnDelete());
    bool rCheckDeletion = !(r.IsEmpty() || r.IsPendingOnDelete());

    if (lCheckDeletion || m_activeTiles.find(lKey) == m_activeTiles.end())
      l.DeleteLater();

    if (rCheckDeletion || m_activeTiles.find(rKey) == m_activeTiles.end())
      r.DeleteLater();

    bool lPendingOnDelete = l.IsPendingOnDelete();
    bool rPendingOnDelete = r.IsPendingOnDelete();

    if (lState == rState && lKey == rKey && !lPendingOnDelete)
      m_needGroupMergeOperation = true;

    if (rPendingOnDelete == lPendingOnDelete)
      return lState < rState;

    if (rPendingOnDelete)
      return true;

    return false;
  }
}
