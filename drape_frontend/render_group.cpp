#include "drape_frontend/render_group.hpp"

#include "base/stl_add.hpp"
#include "geometry/screenbase.hpp"

#include "std/bind.hpp"

namespace df
{

RenderGroup::RenderGroup(dp::GLState const & state, df::TileKey const & tileKey)
  : TBase(state, tileKey)
  , m_pendingOnDelete(false)
{
}

RenderGroup::~RenderGroup()
{
  m_renderBuckets.clear();
}

void RenderGroup::Update(ScreenBase const & modelView)
{
  for(drape_ptr<dp::RenderBucket> const & renderBucket : m_renderBuckets)
    renderBucket->Update(modelView);
}

void RenderGroup::CollectOverlay(ref_ptr<dp::OverlayTree> tree)
{
  for(drape_ptr<dp::RenderBucket> const & renderBucket : m_renderBuckets)
    renderBucket->CollectOverlayHandles(tree);
}

void RenderGroup::Render(ScreenBase const & screen)
{
  ASSERT(m_pendingOnDelete == false, ());
  for(drape_ptr<dp::RenderBucket> const & renderBucket : m_renderBuckets)
    renderBucket->Render(screen);
}

void RenderGroup::PrepareForAdd(size_t countForAdd)
{
  m_renderBuckets.reserve(m_renderBuckets.size() + countForAdd);
}

void RenderGroup::AddBucket(drape_ptr<dp::RenderBucket> && bucket)
{
  m_renderBuckets.push_back(move(bucket));
}

bool RenderGroup::IsLess(RenderGroup const & other) const
{
  return m_state < other.m_state;
}

RenderGroupComparator::RenderGroupComparator()
  : m_needGroupMergeOperation(false)
  , m_needBucketsMergeOperation(false)
{
}

void RenderGroupComparator::ResetInternalState()
{
  m_needBucketsMergeOperation = false;
  m_needGroupMergeOperation = false;
}

bool RenderGroupComparator::operator()(unique_ptr<RenderGroup> const & l, unique_ptr<RenderGroup> const & r)
{
  dp::GLState const & lState = l->GetState();
  dp::GLState const & rState = r->GetState();

  TileKey const & lKey = l->GetTileKey();
  TileKey const & rKey = r->GetTileKey();

  if (!l->IsPendingOnDelete() && l->IsEmpty())
    l->DeleteLater();

  if (!r->IsPendingOnDelete() && r->IsEmpty())
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

UserMarkRenderGroup::UserMarkRenderGroup(dp::GLState const & state,
                                         TileKey const & tileKey,
                                         drape_ptr<dp::RenderBucket> && bucket)
  : TBase(state, tileKey)
  , m_renderBucket(move(bucket))
{
}

UserMarkRenderGroup::~UserMarkRenderGroup()
{
}

void UserMarkRenderGroup::Render(ScreenBase const & screen)
{
  if (m_renderBucket != nullptr)
    m_renderBucket->Render(screen);
}

string DebugPrint(RenderGroup const & group)
{
  ostringstream out;
  out << DebugPrint(group.GetTileKey());
  return out.str();
}

} // namespace df
