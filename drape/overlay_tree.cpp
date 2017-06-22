#include "drape/overlay_tree.hpp"

#include "drape/constants.hpp"
#include "drape/debug_rect_renderer.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"

namespace dp
{
int const kFrameUpdatePeriod = 10;
size_t const kAverageHandlesCount[dp::OverlayRanksCount] = { 300, 200, 50 };
int const kInvalidFrame = -1;

namespace
{
class HandleComparator
{
public:
  HandleComparator(bool enableMask, bool followingMode)
    : m_followingMode(followingMode)
    , m_enableMask(enableMask)
  {}

  bool operator()(ref_ptr<OverlayHandle> const & l, ref_ptr<OverlayHandle> const & r) const
  {
    return IsGreater(l, r);
  }

  bool IsGreater(ref_ptr<OverlayHandle> const & l, ref_ptr<OverlayHandle> const & r) const
  {
    uint64_t const mask = m_enableMask ? l->GetPriorityMask() & r->GetPriorityMask() :
                                         dp::kPriorityMaskAll;
    uint64_t const priorityLeft = (m_followingMode ? l->GetPriorityInFollowingMode() :
                                                     l->GetPriority()) & mask;
    uint64_t const priorityRight = (m_followingMode ? r->GetPriorityInFollowingMode() :
                                                      r->GetPriority()) & mask;
    if (priorityLeft > priorityRight)
      return true;

    if (priorityLeft == priorityRight)
    {
      auto const & hashLeft = l->GetOverlayID();
      auto const & hashRight = r->GetOverlayID();

      if (hashLeft > hashRight)
        return true;

      if (hashLeft == hashRight)
        return l.get() > r.get();
    }

    return false;
  }

private:
  bool m_followingMode;
  bool m_enableMask;
};

void StoreDisplacementInfo(ScreenBase const & modelView, int caseIndex,
                           ref_ptr<OverlayHandle> displacingHandle,
                           ref_ptr<OverlayHandle> displacedHandle,
                           OverlayTree::TDisplacementInfo & displacementInfo)
{
#ifdef DEBUG_OVERLAYS_OUTPUT
  LOG(LINFO, ("Displace (", caseIndex, "):", displacingHandle->GetOverlayDebugInfo(),
              "->", displacedHandle->GetOverlayDebugInfo()));
#else
  UNUSED_VALUE(caseIndex);
#endif

  if (!dp::DebugRectRenderer::Instance().IsEnabled())
    return;
  displacementInfo.emplace_back(displacingHandle->GetExtendedPixelRect(modelView).Center(),
                                displacedHandle->GetExtendedPixelRect(modelView).Center(),
                                dp::Color(0, 0, 255, 255));
}
}  // namespace

OverlayTree::OverlayTree()
  : m_frameCounter(kInvalidFrame)
  , m_followingMode(false)
  , m_isDisplacementEnabled(true)
{
  for (size_t i = 0; i < m_handles.size(); i++)
    m_handles[i].reserve(kAverageHandlesCount[i]);
}

void OverlayTree::Clear()
{
  m_frameCounter = kInvalidFrame;
  TBase::Clear();
  m_handlesCache.clear();
  for (auto & handles : m_handles)
    handles.clear();
}

bool OverlayTree::Frame()
{
  if (IsNeedUpdate())
    return true;

  m_frameCounter++;
  if (m_frameCounter >= kFrameUpdatePeriod)
    m_frameCounter = kInvalidFrame;

  return IsNeedUpdate();
}

bool OverlayTree::IsNeedUpdate() const
{
  return m_frameCounter == kInvalidFrame;
}

void OverlayTree::StartOverlayPlacing(ScreenBase const & screen)
{
  ASSERT(IsNeedUpdate(), ());
  TBase::Clear();
  m_handlesCache.clear();
  m_traits.m_modelView = screen;
  m_displacementInfo.clear();
}

void OverlayTree::Remove(ref_ptr<OverlayHandle> handle)
{
  if (m_frameCounter == kInvalidFrame)
    return;

  if (m_handlesCache.find(handle) != m_handlesCache.end())
    m_frameCounter = kInvalidFrame;
}

void OverlayTree::Add(ref_ptr<OverlayHandle> handle)
{
  ASSERT(IsNeedUpdate(), ());

  ScreenBase const & modelView = GetModelView();

  handle->SetIsVisible(false);
  handle->SetCachingEnable(true);

  // Skip duplicates.
  if (m_handlesCache.find(handle) != m_handlesCache.end())
    return;

  // Skip not-ready handles.
  if (!handle->Update(modelView))
  {
    handle->SetReady(false);
    return;
  }
  else
  {
    handle->SetReady(true);
  }

  // Clip handles which are out of screen.
  double const kScreenRectScale = 1.2;
  m2::RectD const pixelRect = handle->GetExtendedPixelRect(modelView);
  if (modelView.isPerspective())
  {
    m2::RectD screenRect = modelView.PixelRectIn3d();
    screenRect.Scale(kScreenRectScale);
    if (!screenRect.IsIntersect(pixelRect) || modelView.IsReverseProjection3d(pixelRect.Center()))
    {
      handle->SetIsVisible(false);
      return;
    }
  }
  else
  {
    m2::RectD screenRect = modelView.PixelRect();
    screenRect.Scale(kScreenRectScale);
    if (!screenRect.IsIntersect(pixelRect))
    {
      handle->SetIsVisible(false);
      return;
    }
  }

  ASSERT_GREATER_OR_EQUAL(handle->GetOverlayRank(), 0, ());
  size_t const rank = static_cast<size_t>(handle->GetOverlayRank());
  ASSERT_LESS(rank, m_handles.size(), ());
  m_handles[rank].emplace_back(handle);
}

void OverlayTree::InsertHandle(ref_ptr<OverlayHandle> handle, int currentRank,
                               ref_ptr<OverlayHandle> const & parentOverlay)
{
  ASSERT(IsNeedUpdate(), ());

#ifdef DEBUG_OVERLAYS_OUTPUT
  string str = handle->GetOverlayDebugInfo();
  if (!str.empty())
    LOG(LINFO, (str));
#endif

  ScreenBase const & modelView = GetModelView();
  m2::RectD const pixelRect = handle->GetExtendedPixelRect(modelView);
  if (!m_isDisplacementEnabled)
  {
    m_handlesCache.insert(handle);
    TBase::Add(handle, pixelRect);
    return;
  }

  TOverlayContainer rivals;
  HandleComparator comparator(true /* enableMask */, m_followingMode);

  // Find elements that already on OverlayTree and it's pixel rect
  // intersect with handle pixel rect ("Intersected elements").
  ForEachInRect(pixelRect, [&] (ref_ptr<OverlayHandle> const & h)
  {
    bool const isParent = (h == parentOverlay) ||
                          (h->GetOverlayID() == handle->GetOverlayID() &&
                           h->GetOverlayRank() < handle->GetOverlayRank());
    if (!isParent && handle->IsIntersect(modelView, h))
      rivals.push_back(h);
  });

  // If handle is bound to its parent, parent's handle will be used.
  ref_ptr<OverlayHandle> handleToCompare = handle;
  bool const boundToParent = (parentOverlay != nullptr && handle->IsBound());
  if (boundToParent)
    handleToCompare = parentOverlay;

  bool const selected = m_selectedFeatureID.IsValid() &&
                        handleToCompare->GetOverlayID().m_featureId == m_selectedFeatureID;

  if (!selected)
  {
    // In this loop we decide which element must be visible.
    // If input element "handle" has more priority than all "Intersected elements",
    // then we remove all "Intersected elements" and insert input element "handle".
    // But if some of already inserted elements have more priority, then we don't insert "handle".
    for (auto const & rivalHandle : rivals)
    {
      bool const rejectBySelected = m_selectedFeatureID.IsValid() &&
                                    rivalHandle->GetOverlayID().m_featureId == m_selectedFeatureID;

      bool rejectByDepth = false;
      if (!rejectBySelected && modelView.isPerspective())
      {
        bool const pathTextComparation = handle->HasLinearFeatureShape() || rivalHandle->HasLinearFeatureShape();
        rejectByDepth = !pathTextComparation &&
                        handleToCompare->GetPivot(modelView, true).y > rivalHandle->GetPivot(modelView, true).y;
      }

      if (rejectBySelected || rejectByDepth || comparator.IsGreater(rivalHandle, handleToCompare))
      {
        // Handle is displaced and bound to its parent, parent will be displaced too.
        if (boundToParent)
        {
          DeleteHandleWithParents(parentOverlay, currentRank - 1);
          StoreDisplacementInfo(modelView, 0 /* case index */, handle, parentOverlay, m_displacementInfo);
        }
        StoreDisplacementInfo(modelView, 1 /* case index */, rivalHandle, handle, m_displacementInfo);
        return;
      }
    }
  }

  // Current overlay displaces other overlay, delete them.
  for (auto const & rivalHandle : rivals)
  {
    if (rivalHandle->IsBound())
    {
      // Delete rival handle and all handles bound to it.
      for (auto it = m_handlesCache.begin(); it != m_handlesCache.end();)
      {
        if ((*it)->GetOverlayID() == rivalHandle->GetOverlayID())
        {
          Erase(*it);
          StoreDisplacementInfo(modelView, 2 /* case index */, handle, *it, m_displacementInfo);
          it = m_handlesCache.erase(it);
        }
        else
        {
          ++it;
        }
      }
    }
    else
    {
      DeleteHandle(rivalHandle);
      StoreDisplacementInfo(modelView, 3 /* case index */, handle, rivalHandle, m_displacementInfo);
    }
  }

  m_handlesCache.insert(handle);
  TBase::Add(handle, pixelRect);
}

void OverlayTree::EndOverlayPlacing()
{
  ASSERT(IsNeedUpdate(), ());

#ifdef DEBUG_OVERLAYS_OUTPUT
  LOG(LINFO, ("- BEGIN OVERLAYS PLACING"));
#endif

  HandleComparator comparator(false /* enableMask */, m_followingMode);

  for (int rank = 0; rank < dp::OverlayRanksCount; rank++)
  {
    sort(m_handles[rank].begin(), m_handles[rank].end(), comparator);
    for (auto const & handle : m_handles[rank])
    {
      ref_ptr<OverlayHandle> parentOverlay;
      if (!CheckHandle(handle, rank, parentOverlay))
        continue;

      InsertHandle(handle, rank, parentOverlay);
    }
  }
  
  for (int rank = 0; rank < dp::OverlayRanksCount; rank++)
    m_handles[rank].clear();

  for (auto const & handle : m_handlesCache)
  {
    handle->SetIsVisible(true);
    handle->SetCachingEnable(false);
  }

  m_frameCounter = 0;

#ifdef DEBUG_OVERLAYS_OUTPUT
  LOG(LINFO, ("- END OVERLAYS PLACING"));
#endif
}

bool OverlayTree::CheckHandle(ref_ptr<OverlayHandle> handle, int currentRank,
                              ref_ptr<OverlayHandle> & parentOverlay) const
{
  if (currentRank == dp::OverlayRank0)
    return true;

  parentOverlay = FindParent(handle, currentRank - 1);
  return parentOverlay != nullptr;
}

ref_ptr<OverlayHandle> OverlayTree::FindParent(ref_ptr<OverlayHandle> handle, int searchingRank) const
{
  ASSERT_GREATER_OR_EQUAL(searchingRank, 0, ());
  ASSERT_LESS(searchingRank, static_cast<int>(m_handles.size()), ());
  for (auto const & h : m_handles[searchingRank])
  {
    if (h->GetOverlayID() == handle->GetOverlayID() && m_handlesCache.find(h) != m_handlesCache.end())
      return h;
  }
  return nullptr;
}

void OverlayTree::DeleteHandle(ref_ptr<OverlayHandle> const & handle)
{
  size_t const deletedCount = m_handlesCache.erase(handle);
  if (deletedCount != 0)
    Erase(handle);
}

void OverlayTree::DeleteHandleWithParents(ref_ptr<OverlayHandle> handle, int currentRank)
{
  currentRank--;
  while (currentRank >= dp::OverlayRank0)
  {
    auto parent = FindParent(handle, currentRank);
    if (parent != nullptr && parent->IsBound())
      DeleteHandle(parent);
    currentRank--;
  }
  DeleteHandle(handle);
}

bool OverlayTree::GetSelectedFeatureRect(ScreenBase const & screen, m2::RectD & featureRect)
{
  if (!m_selectedFeatureID.IsValid())
    return false;

  featureRect.MakeEmpty();
  for (auto const & handle : m_handlesCache)
  {
    if (handle->IsVisible() && handle->GetOverlayID().m_featureId == m_selectedFeatureID)
    {
      m2::RectD rect = handle->GetPixelRect(screen, screen.isPerspective());
      featureRect.Add(rect);
    }
  }
  return true;
}

void OverlayTree::Select(m2::PointD const & glbPoint, TOverlayContainer & result) const
{
  ScreenBase const & screen = m_traits.m_modelView;
  m2::PointD const pxPoint = screen.GtoP(glbPoint);

  double const kSearchRectHalfSize = 10.0;
  m2::RectD rect(pxPoint, pxPoint);
  rect.Inflate(kSearchRectHalfSize, kSearchRectHalfSize);

  for (auto const & handle : m_handlesCache)
  {
    if (!handle->HasLinearFeatureShape() && rect.IsPointInside(handle->GetPivot(screen, false)))
      result.push_back(handle);
  }
}

void OverlayTree::Select(m2::RectD const & rect, TOverlayContainer & result) const
{
  ScreenBase screen = m_traits.m_modelView;
  ForEachInRect(rect, [&](ref_ptr<OverlayHandle> const & h)
  {
    if (!h->HasLinearFeatureShape() && h->IsVisible() && h->GetOverlayID().m_featureId.IsValid())
    {
      OverlayHandle::Rects shape;
      h->GetPixelShape(screen, screen.isPerspective(), shape);
      for (m2::RectF const & rShape : shape)
      {
        if (rShape.IsIntersect(m2::RectF(rect)))
        {
          result.push_back(h);
          break;
        }
      }
    }
  });
}

void OverlayTree::SetFollowingMode(bool mode)
{
  m_followingMode = mode;
}

void OverlayTree::SetDisplacementEnabled(bool enabled)
{
  m_isDisplacementEnabled = enabled;
  m_frameCounter = kInvalidFrame;
}

void OverlayTree::SetSelectedFeature(FeatureID const & featureID)
{
  m_selectedFeatureID = featureID;
}

OverlayTree::TDisplacementInfo const & OverlayTree::GetDisplacementInfo() const
{
  return m_displacementInfo;
}
}  // namespace dp
