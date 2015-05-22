#include "drape_frontend/render_group.hpp"

#include "base/stl_add.hpp"
#include "geometry/screenbase.hpp"

#include "std/bind.hpp"

namespace df
{

void BaseRenderGroup::UpdateAnimation()
{
  double opactity = 1.0;
  if (m_disappearAnimation != nullptr)
    opactity = m_disappearAnimation->GetOpacity();

  m_uniforms.SetFloatValue("u_opacity", opactity);
}

double BaseRenderGroup::GetOpacity() const
{
  if (m_disappearAnimation != nullptr)
    return m_disappearAnimation->GetOpacity();

  return 1.0;
}

bool BaseRenderGroup::IsAnimating() const
{
  if (m_disappearAnimation == nullptr || m_disappearAnimation->IsFinished())
    return false;

  return true;
}

void BaseRenderGroup::Disappear()
{
  m_disappearAnimation = make_unique<OpacityAnimation>(0.25 /* duration */,
                                                       1.0 /* startOpacity */,
                                                       0.0 /* endOpacity */);
}

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
  for(drape_ptr<dp::RenderBucket> & renderBucket : m_renderBuckets)
    renderBucket->Update(modelView);
}

void RenderGroup::CollectOverlay(ref_ptr<dp::OverlayTree> tree)
{
  for(drape_ptr<dp::RenderBucket> & renderBucket : m_renderBuckets)
    renderBucket->CollectOverlayHandles(tree);
}

void RenderGroup::Render(ScreenBase const & screen)
{
  for(drape_ptr<dp::RenderBucket> & renderBucket : m_renderBuckets)
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

bool RenderGroupComparator::operator()(drape_ptr<RenderGroup> const & l, drape_ptr<RenderGroup> const & r)
{
  dp::GLState const & lState = l->GetState();
  dp::GLState const & rState = r->GetState();

  if (!l->IsPendingOnDelete() && l->IsEmpty())
    l->DeleteLater();

  if (!r->IsPendingOnDelete() && r->IsEmpty())
    r->DeleteLater();

  bool lPendingOnDelete = l->IsPendingOnDelete();
  bool rPendingOnDelete = r->IsPendingOnDelete();

  if (rPendingOnDelete == lPendingOnDelete)
  {
    if (l->GetOpacity() == r->GetOpacity())
      return lState < rState;
    else
      return l->GetOpacity() > r->GetOpacity();
  }

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
