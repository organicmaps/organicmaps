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
  DeleteRange(m_renderBuckets, dp::MasterPointerDeleter());
}

void RenderGroup::Update(ScreenBase const & modelView)
{
  for_each(m_renderBuckets.begin(), m_renderBuckets.end(), bind(&dp::RenderBucket::Update,
                                                                bind(&dp::NonConstGetter<dp::RenderBucket>, _1),
                                                                modelView));
}

void RenderGroup::CollectOverlay(dp::RefPointer<dp::OverlayTree> tree)
{
  for_each(m_renderBuckets.begin(), m_renderBuckets.end(), bind(&dp::RenderBucket::CollectOverlayHandles,
                                                                bind(&dp::NonConstGetter<dp::RenderBucket>, _1),
                                                                tree));
}

void RenderGroup::Render(ScreenBase const & screen)
{
  ASSERT(m_pendingOnDelete == false, ());
  for_each(m_renderBuckets.begin(), m_renderBuckets.end(), bind(&dp::RenderBucket::Render,
                                                                bind(&dp::NonConstGetter<dp::RenderBucket>, _1), screen));
}

void RenderGroup::PrepareForAdd(size_t countForAdd)
{
  m_renderBuckets.reserve(m_renderBuckets.size() + countForAdd);
}

void RenderGroup::AddBucket(dp::TransferPointer<dp::RenderBucket> bucket)
{
  m_renderBuckets.push_back(dp::MasterPointer<dp::RenderBucket>(bucket));
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

UserMarkRenderGroup::UserMarkRenderGroup(dp::GLState const & state, TileKey const & tileKey)
  : TBase(state, tileKey)
  , m_isVisible(true)
{
}

UserMarkRenderGroup::~UserMarkRenderGroup()
{
  m_renderBucket.Destroy();
}

void UserMarkRenderGroup::SetIsVisible(bool isVisible)
{
  m_isVisible = isVisible;
}

bool UserMarkRenderGroup::IsVisible()
{
  return m_isVisible && !m_renderBucket.IsNull();
}

void UserMarkRenderGroup::SetRenderBucket(dp::GLState const & state, dp::TransferPointer<dp::RenderBucket> bucket)
{
  m_state = state;
  m_renderBucket = dp::MasterPointer<dp::RenderBucket>(bucket);
}

void UserMarkRenderGroup::Render(ScreenBase const & screen)
{
  if (!m_renderBucket.IsNull())
    m_renderBucket->Render(screen);
}

} // namespace df
