#pragma once

#include "drape/overlay_handle.hpp"

#include "geometry/screenbase.hpp"
#include "geometry/tree4d.hpp"

#include "base/buffer_vector.hpp"

#include "std/array.hpp"
#include "std/vector.hpp"
#include "std/unordered_map.hpp"

namespace dp
{

//#define COLLECT_DISPLACEMENT_INFO

namespace detail
{

struct OverlayTraits
{
  ScreenBase m_modelView;

  inline m2::RectD const LimitRect(ref_ptr<OverlayHandle> const & handle)
  {
    return handle->GetExtendedPixelRect(m_modelView);
  }
};

}

using TOverlayContainer = buffer_vector<ref_ptr<OverlayHandle>, 8>;

class OverlayTree : public m4::Tree<ref_ptr<OverlayHandle>, detail::OverlayTraits>
{
  using TBase = m4::Tree<ref_ptr<OverlayHandle>, detail::OverlayTraits>;

public:
  OverlayTree();

  bool Frame();
  bool IsNeedUpdate() const;

  void StartOverlayPlacing(ScreenBase const & screen);
  void Add(ref_ptr<OverlayHandle> handle);
  void Remove(ref_ptr<OverlayHandle> handle);
  void EndOverlayPlacing();

  void Select(m2::RectD const & rect, TOverlayContainer & result) const;
  void Select(m2::PointD const & glbPoint, TOverlayContainer & result) const;

  void SetFollowingMode(bool mode);

#ifdef COLLECT_DISPLACEMENT_INFO
  struct DisplacementData
  {
    m2::PointF m_arrowStart;
    m2::PointF m_arrowEnd;
    dp::Color m_arrowColor;
    DisplacementData(m2::PointF const & arrowStart, m2::PointF const & arrowEnd, dp::Color const & arrowColor)
      : m_arrowStart(arrowStart), m_arrowEnd(arrowEnd), m_arrowColor(arrowColor)
    {}
  };
  using TDisplacementInfo = vector<DisplacementData>;
  TDisplacementInfo const & GetDisplacementInfo() const;
#endif

private:
  ScreenBase const & GetModelView() const { return m_traits.m_modelView; }
  void InsertHandle(ref_ptr<OverlayHandle> handle,
                    ref_ptr<OverlayHandle> const & parentOverlay);
  bool CheckHandle(ref_ptr<OverlayHandle> handle, int currentRank,
                   ref_ptr<OverlayHandle> & parentOverlay) const;
  void DeleteHandle(ref_ptr<OverlayHandle> const & handle);

  uint64_t GetHandleKey(ref_ptr<OverlayHandle> const & handle) const;

  int m_frameCounter;
  array<vector<ref_ptr<OverlayHandle>>, dp::OverlayRanksCount> m_handles;
  vector<ref_ptr<OverlayHandle>> m_handlesToDelete;

  unordered_map<uint64_t, ref_ptr<OverlayHandle>> m_handlesCache;

  bool m_followingMode;

#ifdef COLLECT_DISPLACEMENT_INFO
  TDisplacementInfo m_displacementInfo;
#endif
};

} // namespace dp
