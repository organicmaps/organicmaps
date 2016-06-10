#include "drape/overlay_tree.hpp"

#include "drape/constants.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"

namespace dp
{

int const kFrameUpdatePeriod = 10;
int const kAverageHandlesCount[dp::OverlayRanksCount] = { 300, 200, 50 };
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
      auto const & hashLeft = l->GetFeatureID();
      auto const & hashRight = r->GetFeatureID();

      if (hashLeft < hashRight)
        return true;

      if (hashLeft == hashRight)
        return l.get() < r.get();
    }

    return false;
  }

private:
  bool m_followingMode;
  bool m_enableMask;
};

} // namespace

OverlayTree::OverlayTree()
  : m_frameCounter(kInvalidFrame)
  , m_followingMode(false)
  , m_isDisplacementEnabled(true)
  , m_displacementMode(displacement::kDefaultMode)
{
  for (size_t i = 0; i < m_handles.size(); i++)
    m_handles[i].reserve(kAverageHandlesCount[i]);
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
  Clear();
  m_handlesCache.clear();
  m_traits.m_modelView = screen;

#ifdef COLLECT_DISPLACEMENT_INFO
  m_displacementInfo.clear();
#endif
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

  // Skip overlays from another displacement mode.
  if ((handle->GetDisplacementMode() & m_displacementMode) == 0)
    return;

  // Skip duplicates.
  if (m_handlesCache.find(handle) != m_handlesCache.end())
    return;

  // Skip not-ready handles.
  if (!handle->Update(modelView))
    return;

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

  int const rank = handle->GetOverlayRank();
  ASSERT_LESS(rank, m_handles.size(), ());
  m_handles[rank].emplace_back(handle);
}

void OverlayTree::InsertHandle(ref_ptr<OverlayHandle> handle,
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
                          (h->GetFeatureID() == handle->GetFeatureID() &&
                           h->GetOverlayRank() < handle->GetOverlayRank());
    if (!isParent && handle->IsIntersect(modelView, h))
      rivals.push_back(h);
  });

  // If handle is bound to its parent, parent's handle will be used.
  ref_ptr<OverlayHandle> handleToCompare = handle;
  bool const boundToParent = (parentOverlay != nullptr && handle->IsBound());
  if (boundToParent)
    handleToCompare = parentOverlay;

  // In this loop we decide which element must be visible.
  // If input element "handle" more priority than all "Intersected elements"
  // than we remove all "Intersected elements" and insert input element "handle".
  // But if some of already inserted elements more priority than we don't insert "handle".
  for (auto const & rivalHandle : rivals)
  {
    bool rejectByDepth = false;
    if (modelView.isPerspective())
    {
      bool const pathTextComparation = handle->HasDynamicAttributes() || rivalHandle->HasDynamicAttributes();
      rejectByDepth = !pathTextComparation &&
                      handleToCompare->GetPivot(modelView, true).y > rivalHandle->GetPivot(modelView, true).y;
    }

    if (rejectByDepth || comparator.IsGreater(rivalHandle, handleToCompare))
    {
      // Handle is displaced and bound to its parent, parent will be displaced too.
      if (boundToParent)
      {
        DeleteHandle(parentOverlay);

      #ifdef DEBUG_OVERLAYS_OUTPUT
        LOG(LINFO, ("Displace (0):", handle->GetOverlayDebugInfo(), "->", parentOverlay->GetOverlayDebugInfo()));
      #endif

      #ifdef COLLECT_DISPLACEMENT_INFO
        m_displacementInfo.emplace_back(DisplacementData(handle->GetExtendedPixelRect(modelView).Center(),
                                                         parentOverlay->GetExtendedPixelRect(modelView).Center(),
                                                         dp::Color(0, 255, 0, 255)));
      #endif
      }

    #ifdef DEBUG_OVERLAYS_OUTPUT
      LOG(LINFO, ("Displace (1):", rivalHandle->GetOverlayDebugInfo(), "->", handle->GetOverlayDebugInfo()));
    #endif

    #ifdef COLLECT_DISPLACEMENT_INFO
      m_displacementInfo.emplace_back(DisplacementData(rivalHandle->GetExtendedPixelRect(modelView).Center(),
                                                       handle->GetExtendedPixelRect(modelView).Center(),
                                                       dp::Color(0, 0, 255, 255)));
    #endif
      return;
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
        if ((*it)->GetFeatureID() == rivalHandle->GetFeatureID())
        {
          Erase(*it);

        #ifdef DEBUG_OVERLAYS_OUTPUT
          LOG(LINFO, ("Displace (2):", handle->GetOverlayDebugInfo(), "->", (*it)->GetOverlayDebugInfo()));
        #endif

        #ifdef COLLECT_DISPLACEMENT_INFO
          m_displacementInfo.emplace_back(DisplacementData(handle->GetExtendedPixelRect(modelView).Center(),
                                                           (*it)->GetExtendedPixelRect(modelView).Center(),
                                                           dp::Color(0, 0, 255, 255)));
        #endif

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

    #ifdef DEBUG_OVERLAYS_OUTPUT
      LOG(LINFO, ("Displace (3):", handle->GetOverlayDebugInfo(), "->", rivalHandle->GetOverlayDebugInfo()));
    #endif

    #ifdef COLLECT_DISPLACEMENT_INFO
      m_displacementInfo.emplace_back(DisplacementData(handle->GetExtendedPixelRect(modelView).Center(),
                                                       rivalHandle->GetExtendedPixelRect(modelView).Center(),
                                                       dp::Color(0, 0, 255, 255)));
    #endif
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

      InsertHandle(handle, parentOverlay);
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

  int const seachingRank = currentRank - 1;
  for (auto const & h : m_handles[seachingRank])
  {
    if (h->GetFeatureID() == handle->GetFeatureID() &&
        m_handlesCache.find(h) != m_handlesCache.end())
    {
      parentOverlay = h;
      return true;
    }
  }

  return false;
}

void OverlayTree::DeleteHandle(ref_ptr<OverlayHandle> const & handle)
{
  size_t const deletedCount = m_handlesCache.erase(handle);
  ASSERT_NOT_EQUAL(deletedCount, 0, ());
  if (deletedCount != 0)
    Erase(handle);
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
    if (rect.IsPointInside(handle->GetPivot(screen, false)))
      result.push_back(handle);
  }
}

void OverlayTree::Select(m2::RectD const & rect, TOverlayContainer & result) const
{
  ScreenBase screen = m_traits.m_modelView;
  ForEachInRect(rect, [&](ref_ptr<OverlayHandle> const & h)
  {
    if (h->IsVisible() && h->GetFeatureID().IsValid())
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

void OverlayTree::SetDisplacementMode(int displacementMode)
{
  m_displacementMode = displacementMode;
  m_frameCounter = kInvalidFrame;
}

#ifdef COLLECT_DISPLACEMENT_INFO

OverlayTree::TDisplacementInfo const & OverlayTree::GetDisplacementInfo() const
{
  return m_displacementInfo;
}

#endif

} // namespace dp
