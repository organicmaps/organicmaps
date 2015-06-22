#include "drape_frontend/render_group.hpp"

#include "base/stl_add.hpp"
#include "geometry/screenbase.hpp"

#include "std/bind.hpp"

namespace df
{

void BaseRenderGroup::UpdateAnimation()
{
  m_uniforms.SetFloatValue("u_opacity", 1.0);
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
    renderBucket->CollectOverlayHandles(tree, GetOpacity() < 1.0);
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

void RenderGroup::UpdateAnimation()
{
  double opactity = 1.0;
  if (m_disappearAnimation != nullptr)
    opactity = m_disappearAnimation->GetOpacity();

  m_uniforms.SetFloatValue("u_opacity", opactity);
}

double RenderGroup::GetOpacity() const
{
  if (m_disappearAnimation != nullptr)
    return m_disappearAnimation->GetOpacity();

  return 1.0;
}

bool RenderGroup::IsAnimating() const
{
  if (m_disappearAnimation == nullptr || m_disappearAnimation->IsFinished())
    return false;

  return true;
}

void RenderGroup::Disappear()
{
  m_disappearAnimation = make_unique<OpacityAnimation>(0.25 /* duration */,
                                                       1.0 /* startOpacity */,
                                                       0.0 /* endOpacity */);
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
    dp::GLState::DepthLayer lDepth = lState.GetDepthLayer();
    dp::GLState::DepthLayer rDepth = rState.GetDepthLayer();
    if (lDepth != rDepth)
      return lDepth < rDepth;

    if (my::AlmostEqualULPs(l->GetOpacity(), r->GetOpacity()))
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
  , m_animation(new OpacityAnimation(0.25 /*duration*/, 0.0 /* minValue */, 1.0 /* maxValue*/))
{
  m_mapping.AddRangePoint(0.6, 1.3);
  m_mapping.AddRangePoint(0.85, 0.8);
  m_mapping.AddRangePoint(1.0, 1.0);
}

UserMarkRenderGroup::~UserMarkRenderGroup()
{
}

void UserMarkRenderGroup::UpdateAnimation()
{
  BaseRenderGroup::UpdateAnimation();
  float t = 1.0;
  if (m_animation)
    t = m_animation->GetOpacity();

  m_uniforms.SetFloatValue("u_interpolationT", m_mapping.GetValue(t));
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
