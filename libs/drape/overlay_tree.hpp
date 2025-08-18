#pragma once

#include "drape/drape_diagnostics.hpp"
#include "drape/overlay_handle.hpp"

#include "geometry/screenbase.hpp"
#include "geometry/tree4d.hpp"

#include "base/buffer_vector.hpp"

#include <array>
#include <memory>
#include <unordered_set>
#include <vector>

namespace dp
{
namespace detail
{
class OverlayTraits
{
public:
  m2::RectD const LimitRect(ref_ptr<OverlayHandle> const & handle) { return handle->GetExtendedPixelRect(m_modelView); }
  ScreenBase const & GetModelView() const { return m_modelView; }
  m2::RectD const & GetExtendedScreenRect() const { return m_extendedScreenRect; }
  m2::RectD const & GetDisplacersFreeRect() const { return m_displacersFreeRect; }

  void SetVisualScale(double visualScale);
  void SetModelView(ScreenBase const & modelView);

private:
  double m_visualScale;
  ScreenBase m_modelView;
  m2::RectD m_extendedScreenRect;
  m2::RectD m_displacersFreeRect;
};

struct OverlayHasher
{
  std::hash<OverlayHandle *> m_hasher;

  size_t operator()(ref_ptr<OverlayHandle> const & handle) const { return m_hasher(handle.get()); }
};
}  // namespace detail

class DebugRenderer;

using TOverlayContainer = buffer_vector<ref_ptr<OverlayHandle>, 8>;

class OverlayTree : public m4::Tree<ref_ptr<OverlayHandle>, detail::OverlayTraits>
{
  using TBase = m4::Tree<ref_ptr<OverlayHandle>, detail::OverlayTraits>;

public:
  using HandlesCache = std::unordered_set<ref_ptr<OverlayHandle>, detail::OverlayHasher>;

  explicit OverlayTree(double visualScale);

  void SetVisualScale(double visualScale);

  void Clear();
  bool Frame();
  bool IsNeedUpdate() const;
  void InvalidateOnNextFrame();

  void StartOverlayPlacing(ScreenBase const & screen, uint8_t zoomLevel);
  void Add(ref_ptr<OverlayHandle> handle);
  //! \return true if tree completely invalidated and next call has no sense
  bool Remove(ref_ptr<OverlayHandle> handle);
  void EndOverlayPlacing();

  HandlesCache const & GetHandlesCache() const { return m_handlesCache; }

  void Select(m2::RectD const & rect, TOverlayContainer & result) const;
  void Select(m2::PointD const & glbPoint, TOverlayContainer & result) const;

  void SetDisplacementEnabled(bool enabled);

  void SetSelectedFeature(FeatureID const & featureID);
  bool GetSelectedFeatureRect(ScreenBase const & screen, m2::RectD & featureRect);

  struct DisplacementData
  {
    m2::PointF m_arrowStart;
    m2::PointF m_arrowEnd;
    dp::Color m_arrowColor;
    DisplacementData(m2::PointF const & arrowStart, m2::PointF const & arrowEnd, dp::Color const & arrowColor)
      : m_arrowStart(arrowStart)
      , m_arrowEnd(arrowEnd)
      , m_arrowColor(arrowColor)
    {}
  };
  using TDisplacementInfo = std::vector<DisplacementData>;
  TDisplacementInfo const & GetDisplacementInfo() const;

  void SetDebugRectRenderer(ref_ptr<DebugRenderer> debugRectRenderer);

private:
  ScreenBase const & GetModelView() const { return m_traits.GetModelView(); }
  void InsertHandle(ref_ptr<OverlayHandle> handle, int currentRank, ref_ptr<OverlayHandle> const & parentOverlay);
  bool CheckHandle(ref_ptr<OverlayHandle> handle, int currentRank, ref_ptr<OverlayHandle> & parentOverlay) const;
  bool DeleteHandleImpl(ref_ptr<OverlayHandle> handle);
  void DeleteHandle(ref_ptr<OverlayHandle> handle);

  ref_ptr<OverlayHandle> FindParent(ref_ptr<OverlayHandle> handle, int searchingRank) const;
  void DeleteHandleWithParents(ref_ptr<OverlayHandle> handle, int currentRank);

  void StoreDisplacementInfo(int caseIndex, ref_ptr<OverlayHandle> displacerHandle,
                             ref_ptr<OverlayHandle> displacedHandle);

  bool IsInCache(ref_ptr<OverlayHandle> const & handle) const;

  int m_frameCounter;
  std::array<std::vector<ref_ptr<OverlayHandle>>, dp::OverlayRanksCount> m_handles;

  HandlesCache m_handlesCache;
  /// @todo By VNG: Additional cache by OverlayID for fast FindParent(OverlayHandle).
  /// I did profiling here in perspective-navigation mode.
  /// The best solution is to store parent in OverlayHandle, but I didn't realize
  /// how to implement it in a reasonable time, except "rewrite all".
  /// Probably, another good solution is to combine m_handlesCache and m_overlayIdCache and make
  /// one container like unordered_map<{FeatureID, kml::MarkId}, buffer_vector<OverlayHandle>>.
  std::map<OverlayID, buffer_vector<ref_ptr<OverlayHandle>, 4>> m_overlayIdCache;

  bool m_isDisplacementEnabled;

  FeatureID m_selectedFeatureID;

  TDisplacementInfo m_displacementInfo;
  ref_ptr<DebugRenderer> m_debugRectRenderer;

  HandlesCache m_displacers;
  uint32_t m_frameUpdatePeriod;
  uint8_t m_zoomLevel = 1;
};
}  // namespace dp
