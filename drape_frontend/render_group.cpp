#include "render_group.hpp"

#include "../base/stl_add.hpp"

#include "../std/bind.hpp"

namespace df
{
namespace
{

RenderBucket * NonConstGetter(MasterPointer<RenderBucket> & p)
{
  return p.GetRaw();
}

} /// namespace

RenderGroup::RenderGroup(GLState const & state, df::TileKey const & tileKey)
  : m_state(state)
  , m_tileKey(tileKey)
  , m_pendingOnDelete(false)
{
}

RenderGroup::~RenderGroup()
{
  (void)GetRangeDeletor(m_renderBuckets, MasterPointerDeleter())();
}

void RenderGroup::CollectOverlay(RefPointer<OverlayTree> tree)
{
  for_each(m_renderBuckets.begin(), m_renderBuckets.end(), bind(&RenderBucket::CollectOverlayHandles,
                                                                bind(&NonConstGetter, _1),
                                                                tree));
}

void RenderGroup::Render()
{
  ASSERT(m_pendingOnDelete == false, ());
  for_each(m_renderBuckets.begin(), m_renderBuckets.end(), bind(&RenderBucket::Render,
                                                                bind(&NonConstGetter, _1)));
}

void RenderGroup::PrepareForAdd(size_t countForAdd)
{
  m_renderBuckets.reserve(m_renderBuckets.size() + countForAdd);
}

void RenderGroup::AddBucket(TransferPointer<RenderBucket> bucket)
{
  m_renderBuckets.push_back(MasterPointer<RenderBucket>(bucket));
}

bool RenderGroup::IsLess(RenderGroup const & other) const
{
  return m_state < other.m_state;
}

RenderBucketComparator::RenderBucketComparator(set<TileKey> const & activeTiles)
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

bool RenderBucketComparator::operator()(RenderGroup const * l, RenderGroup const * r)
{
  GLState const & lState = l->GetState();
  GLState const & rState = r->GetState();

  TileKey const & lKey = l->GetTileKey();
  TileKey const & rKey = r->GetTileKey();

  if (!l->IsPendingOnDelete() && (l->IsEmpty() || m_activeTiles.find(lKey) == m_activeTiles.end()))
    l->DeleteLater();

  if (!r->IsPendingOnDelete() && (r->IsEmpty() || m_activeTiles.find(rKey) == m_activeTiles.end()))
    r->DeleteLater();

  bool lPendingOnDelete = l->IsPendingOnDelete();
  bool rPendingOnDelete = r->IsPendingOnDelete();

  if (lState == rState && lKey == rKey && !lPendingOnDelete)
    m_needGroupMergeOperation = true;

  if (rPendingOnDelete == lPendingOnDelete)
    return lState < rState;

  if (rPendingOnDelete)
    return true;

  return false;
}

} // namespace df
