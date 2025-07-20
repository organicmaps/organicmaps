#include "drape/overlay_tree.hpp"

#include "drape/constants.hpp"
#include "drape/debug_renderer.hpp"

#include <algorithm>

namespace dp
{
int const kMinFrameUpdatePeriod = 5;
int const kAvgFrameUpdatePeriod = 10;
int const kMaxFrameUpdatePeriod = 15;
uint32_t const kMinHandlesCount = 100;
uint32_t const kMaxHandlesCount = 1000;

size_t const kAverageHandlesCount[dp::OverlayRanksCount] = { 300, 200, 50 };
int const kInvalidFrame = -1;

namespace
{
class HandleComparator
{
public:
  explicit HandleComparator(bool enableMask)
    : m_enableMask(enableMask)
  {}

  bool operator()(ref_ptr<OverlayHandle> const & l, ref_ptr<OverlayHandle> const & r) const
  {
    return IsGreater(l, r);
  }

  bool IsGreater(ref_ptr<OverlayHandle> const & l, ref_ptr<OverlayHandle> const & r) const
  {
    bool const displayFlagLeft = ((!m_enableMask || l->IsSpecialLayerOverlay()) ? true : l->GetDisplayFlag());
    bool const displayFlagRight = ((!m_enableMask || r->IsSpecialLayerOverlay()) ? true : r->GetDisplayFlag());
    if (displayFlagLeft > displayFlagRight)
      return true;

    if (displayFlagLeft == displayFlagRight)
    {
      uint64_t const priorityLeft = l->GetPriority();
      uint64_t const priorityRight = r->GetPriority();
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
    }
    return false;
  }

  bool IsEqual(ref_ptr<OverlayHandle> const & l, ref_ptr<OverlayHandle> const & r) const
  {
    bool const displayFlagLeft = ((!m_enableMask || l->IsSpecialLayerOverlay()) ? true : l->GetDisplayFlag());
    bool const displayFlagRight = ((!m_enableMask || r->IsSpecialLayerOverlay()) ? true : r->GetDisplayFlag());

    if (displayFlagLeft == displayFlagRight)
      return l->GetPriority() == r->GetPriority();

    return false;
  }

private:
  bool m_enableMask;
};
}  // namespace

OverlayTree::OverlayTree(double visualScale)
  : m_frameCounter(kInvalidFrame)
  , m_isDisplacementEnabled(true)
  , m_frameUpdatePeriod(kMinFrameUpdatePeriod)
{
  m_traits.SetVisualScale(visualScale);
  for (size_t i = 0; i < m_handles.size(); i++)
    m_handles[i].reserve(kAverageHandlesCount[i]);
}

void OverlayTree::SetVisualScale(double visualScale)
{
  m_traits.SetVisualScale(visualScale);
  InvalidateOnNextFrame();
}

void OverlayTree::Clear()
{
  InvalidateOnNextFrame();
  TBase::Clear();
  m_handlesCache.clear();
  m_overlayIdCache.clear();
  for (auto & handles : m_handles)
    handles.clear();
  m_displacers.clear();
}

bool OverlayTree::Frame()
{
  if (IsNeedUpdate())
    return true;

  // Choose optimal frame update period.
  if (m_frameCounter == 0)
  {
    auto const handlesCount = m_handlesCache.size();
    if (handlesCount > kMaxHandlesCount)
      m_frameUpdatePeriod = kMaxFrameUpdatePeriod;
    else if (handlesCount < kMinHandlesCount)
      m_frameUpdatePeriod = kMinFrameUpdatePeriod;
    else
      m_frameUpdatePeriod = kAvgFrameUpdatePeriod;
  }

  m_frameCounter++;
  if (m_frameCounter >= static_cast<int>(m_frameUpdatePeriod))
    InvalidateOnNextFrame();

  return IsNeedUpdate();
}

bool OverlayTree::IsNeedUpdate() const
{
  return m_frameCounter == kInvalidFrame;
}

void OverlayTree::InvalidateOnNextFrame()
{
  m_frameCounter = kInvalidFrame;
}

void OverlayTree::StartOverlayPlacing(ScreenBase const & screen, uint8_t zoomLevel)
{
  ASSERT(IsNeedUpdate(), ());
  TBase::Clear();
  m_handlesCache.clear();
  m_overlayIdCache.clear();
  m_traits.SetModelView(screen);
  m_displacementInfo.clear();
  m_zoomLevel = zoomLevel;
}

bool OverlayTree::Remove(ref_ptr<OverlayHandle> handle)
{
  if (m_frameCounter == kInvalidFrame)
  {
    if (!IsEmpty())
      Clear();
    return true;
  }

  if (IsInCache(handle))
  {
    Clear();
    return true;
  }
  return false;
}

void OverlayTree::Add(ref_ptr<OverlayHandle> handle)
{
  /// @todo Fires when deleting downloaded country ?!
  //ASSERT(handle->GetOverlayID().IsValid(), ());
  ASSERT(IsNeedUpdate(), ());

  ScreenBase const & modelView = GetModelView();

  handle->SetIsVisible(false);

  if (m_zoomLevel < handle->GetMinVisibleScale())
    return;

  handle->EnableCaching(true);

  // Skip duplicates.
  if (IsInCache(handle))
    return;

  // Skip not-ready handles.
  if (!handle->Update(modelView))
  {
    InvalidateOnNextFrame();
    handle->SetReady(false);
    return;
  }
  else
  {
    handle->SetReady(true);
  }

  // Clip handles which are out of screen if these handles were not displacers
  // last time. Also clip all handles in reverse projection.
  m2::RectD const pixelRect = handle->GetExtendedPixelRect(modelView);
  if (modelView.IsReverseProjection3d(pixelRect.Center()) ||
      (m_displacers.find(handle) == m_displacers.end() &&
       !m_traits.GetExtendedScreenRect().IsIntersect(pixelRect)))
  {
    handle->SetIsVisible(false);
    return;
  }

  ASSERT_GREATER_OR_EQUAL(handle->GetOverlayRank(), 0, ());
  size_t const rank = static_cast<size_t>(handle->GetOverlayRank());
  ASSERT_LESS(rank, m_handles.size(), ());
  m_handles[rank].emplace_back(std::move(handle));
}

void OverlayTree::InsertHandle(ref_ptr<OverlayHandle> handle, int currentRank,
                               ref_ptr<OverlayHandle> const & parentOverlay)
{
  /// @todo Fires when updating country (delete-add) ?!
  //ASSERT(handle->GetOverlayID().IsValid(), ());
  ASSERT(IsNeedUpdate(), ());

#ifdef DEBUG_OVERLAYS_OUTPUT
  std::string str = handle->GetOverlayDebugInfo();
  if (!str.empty())
    LOG(LINFO, (str));
#endif

  ScreenBase const & modelView = GetModelView();
  ASSERT(handle->IsCachingEnabled(), ());
  m2::RectD const pixelRect = handle->GetExtendedPixelRect(modelView);

  if (!m_isDisplacementEnabled)
  {
    m_handlesCache.insert(handle);
    m_overlayIdCache[handle->GetOverlayID()].push_back(handle);
    TBase::Add(handle, pixelRect);
    return;
  }

  TOverlayContainer rivals;
  HandleComparator comparator(true /* enableMask */);

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
      bool reject = m_selectedFeatureID.IsValid() && rivalHandle->GetOverlayID().m_featureId == m_selectedFeatureID;
      if (!reject)
      {
        if (modelView.isPerspective())
        {
          bool const isEqual = comparator.IsEqual(rivalHandle, handleToCompare);
          bool const pathTextComparation = handle->HasLinearFeatureShape() || rivalHandle->HasLinearFeatureShape();
          bool const specialLayerComparation = handle->IsSpecialLayerOverlay() || rivalHandle->IsSpecialLayerOverlay();

          if (isEqual && !pathTextComparation && !specialLayerComparation)
            reject = handleToCompare->GetPivot(modelView, true).y < rivalHandle->GetPivot(modelView, true).y;
          else
            reject = comparator.IsGreater(rivalHandle, handleToCompare);
        }
        else
        {
          reject = comparator.IsGreater(rivalHandle, handleToCompare);
        }
      }

      if (reject)
      {
        // Handle is displaced and bound to its parent, parent will be displaced too.
        if (boundToParent)
        {
          DeleteHandleWithParents(parentOverlay, currentRank - 1);
          StoreDisplacementInfo(0 /* case index */, handle, parentOverlay);
        }
        StoreDisplacementInfo(1 /* case index */, rivalHandle, handle);
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
      auto it = m_overlayIdCache.find(rivalHandle->GetOverlayID());
      if (it != m_overlayIdCache.end())
      {
        for (auto const & h : it->second)
        {
          DeleteHandleImpl(h);
          StoreDisplacementInfo(2 /* case index */, handle, h);
        }
        m_overlayIdCache.erase(it);
      }
    }
    else
    {
      DeleteHandle(rivalHandle);
      StoreDisplacementInfo(3 /* case index */, handle, rivalHandle);
    }
  }

  m_handlesCache.insert(handle);
  m_overlayIdCache[handle->GetOverlayID()].push_back(handle);
  TBase::Add(handle, pixelRect);
}

void OverlayTree::EndOverlayPlacing()
{
  ASSERT(IsNeedUpdate(), ());

  m_displacers.clear();

#ifdef DEBUG_OVERLAYS_OUTPUT
  LOG(LINFO, ("- BEGIN OVERLAYS PLACING"));
#endif

  HandleComparator comparator(false /* enableMask */);

  for (int rank = 0; rank < dp::OverlayRanksCount; rank++)
  {
    std::sort(m_handles[rank].begin(), m_handles[rank].end(), comparator);

    for (auto const & handle : m_handles[rank])
    {
      ref_ptr<OverlayHandle> parentOverlay;
      if (CheckHandle(handle, rank, parentOverlay))
        InsertHandle(handle, rank, parentOverlay);
    }
  }

  for (int rank = 0; rank < dp::OverlayRanksCount; rank++)
  {
    for (auto const & handle : m_handles[rank])
      handle->SetDisplayFlag(false);
    m_handles[rank].clear();
  }

  for (auto const & handle : m_handlesCache)
  {
    handle->SetDisplayFlag(true);
    handle->SetIsVisible(true);
    handle->EnableCaching(false);
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

  auto it = m_overlayIdCache.find(handle->GetOverlayID());
  if (it != m_overlayIdCache.end())
  {
    for (auto const & h : it->second)
      if (h->GetOverlayRank() == searchingRank)
        return h;
  }
  return nullptr;
}

bool OverlayTree::DeleteHandleImpl(ref_ptr<OverlayHandle> handle)
{
  if (m_handlesCache.erase(handle) > 0)
  {
    Erase(handle);
    return true;
  }
  return false;
}

void OverlayTree::DeleteHandle(ref_ptr<OverlayHandle> handle)
{
  if (DeleteHandleImpl(handle))
  {
    auto it = m_overlayIdCache.find(handle->GetOverlayID());
    ASSERT(it != m_overlayIdCache.end(), ());

    auto & v = it->second;
    v.erase_if([&handle](ref_ptr<OverlayHandle> const & h) { return handle == h; });
    if (v.empty())
      m_overlayIdCache.erase(it);
  }
}

void OverlayTree::DeleteHandleWithParents(ref_ptr<OverlayHandle> handle, int currentRank)
{
  auto it = m_overlayIdCache.find(handle->GetOverlayID());
  ASSERT(it != m_overlayIdCache.end(), ());

  auto & v = it->second;
  v.erase_if([&](ref_ptr<OverlayHandle> const & h)
  {
    if (h == handle || (h->GetOverlayRank() < currentRank && h->IsBound()))
    {
      DeleteHandleImpl(h);
      return true;
    }
    return false;
  });

  if (v.empty())
    m_overlayIdCache.erase(it);
}

bool OverlayTree::GetSelectedFeatureRect(ScreenBase const & screen, m2::RectD & featureRect)
{
  if (!m_selectedFeatureID.IsValid())
    return false;

  auto resultRect = m2::RectD::GetEmptyRect();
  for (auto it = m_overlayIdCache.lower_bound(OverlayID::GetLowerKey(m_selectedFeatureID));
       it != m_overlayIdCache.end() && it->first.m_featureId == m_selectedFeatureID; ++it)
  {
    for (auto const & handle : it->second)
    {
      if (handle->IsVisible())
        resultRect.Add(handle->GetPixelRect(screen, screen.isPerspective()));
    }
  }

  if (resultRect.IsValid())
  {
    featureRect = resultRect;
    return true;
  }

  return false;
}

void OverlayTree::Select(m2::PointD const & glbPoint, TOverlayContainer & result) const
{
  ScreenBase const & screen = GetModelView();
  m2::PointD const pxPoint = screen.GtoP(glbPoint);

  double const kSearchRectHalfSize = 10.0;
  m2::RectD rect(pxPoint, pxPoint);
  rect.Inflate(kSearchRectHalfSize, kSearchRectHalfSize);

  /// @todo Why we can't call Select(rect) here?
  /// Why we don't check handle->IsVisible(), like in Select(rect)?
  for (auto const & handle : m_handlesCache)
  {
    if (!handle->HasLinearFeatureShape() && rect.IsPointInside(handle->GetPivot(screen, false)))
      result.push_back(handle);
  }
}

void OverlayTree::Select(m2::RectD const & rect, TOverlayContainer & result) const
{
  ScreenBase screen = GetModelView();
  ForEachInRect(rect, [&](ref_ptr<OverlayHandle> const & h)
  {
    ASSERT(h->GetOverlayID().IsValid(), ());

    if (!h->HasLinearFeatureShape() && h->IsVisible())
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

void OverlayTree::SetDisplacementEnabled(bool enabled)
{
  if (m_isDisplacementEnabled == enabled)
    return;
  m_isDisplacementEnabled = enabled;
  InvalidateOnNextFrame();
}

void OverlayTree::SetSelectedFeature(FeatureID const & featureID)
{
  m_selectedFeatureID = featureID;
}

OverlayTree::TDisplacementInfo const & OverlayTree::GetDisplacementInfo() const
{
  return m_displacementInfo;
}

void OverlayTree::SetDebugRectRenderer(ref_ptr<DebugRenderer> debugRectRenderer)
{
  m_debugRectRenderer = debugRectRenderer;
}

void OverlayTree::StoreDisplacementInfo(int caseIndex, ref_ptr<OverlayHandle> displacerHandle,
                                        ref_ptr<OverlayHandle> displacedHandle)
{
  ScreenBase const & modelView = GetModelView();
  m2::RectD const pixelRect = displacerHandle->GetExtendedPixelRect(modelView);
  if (!m_traits.GetDisplacersFreeRect().IsRectInside(pixelRect))
    m_displacers.insert(displacerHandle);

#ifdef DEBUG_OVERLAYS_OUTPUT
    LOG(LINFO, ("Displace (", caseIndex, "):", displacerHandle->GetOverlayDebugInfo(),
                "->", displacedHandle->GetOverlayDebugInfo()));
#else
  UNUSED_VALUE(caseIndex);
#endif

  if (m_debugRectRenderer && m_debugRectRenderer->IsEnabled())
  {
    m_displacementInfo.emplace_back(m2::PointF(displacerHandle->GetExtendedPixelRect(modelView).Center()),
                                    m2::PointF(displacedHandle->GetExtendedPixelRect(modelView).Center()),
                                    dp::Color(0, 0, 255, 255));
  }
}

bool OverlayTree::IsInCache(ref_ptr<OverlayHandle> const & handle) const
{
  return (m_handlesCache.find(handle) != m_handlesCache.end());
}

void detail::OverlayTraits::SetVisualScale(double visualScale)
{
  m_visualScale = visualScale;
}

void detail::OverlayTraits::SetModelView(ScreenBase const & modelView)
{
  m_modelView = modelView;

  double const extension = m_visualScale * kScreenPixelRectExtension;
  double const doubleExtension = 2.0 * extension;
  m_extendedScreenRect = modelView.PixelRectIn3d();
  m_extendedScreenRect.Inflate(extension, extension);

  m_displacersFreeRect = modelView.PixelRectIn3d();
  if (m_displacersFreeRect.SizeX() > doubleExtension && m_displacersFreeRect.SizeY() > doubleExtension)
    m_displacersFreeRect.Inflate(-extension, -extension);
  else
    m_displacersFreeRect.SetSizes(1e-7, 1e-7);
}
}  // namespace dp
