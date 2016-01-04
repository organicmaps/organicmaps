#pragma once

#include "drape/overlay_handle.hpp"

#include "geometry/screenbase.hpp"
#include "geometry/tree4d.hpp"

#include "base/buffer_vector.hpp"

#include "std/array.hpp"
#include "std/vector.hpp"

namespace dp
{

//#define COLLECT_DISPLACEMENT_INFO

namespace detail
{

struct OverlayInfo
{
  ref_ptr<OverlayHandle> m_handle;

  OverlayInfo() = default;
  OverlayInfo(ref_ptr<OverlayHandle> handle)
    : m_handle(handle)
  {}

  bool operator==(OverlayInfo const & rhs) const
  {
    return m_handle == rhs.m_handle;
  }
};

struct OverlayTraits
{
  ScreenBase m_modelView;

  inline m2::RectD const LimitRect(OverlayInfo const & info)
  {
    return info.m_handle->GetExtendedPixelRect(m_modelView);
  }
};

}

class OverlayTree : public m4::Tree<detail::OverlayInfo, detail::OverlayTraits>
{
  using TBase = m4::Tree<detail::OverlayInfo, detail::OverlayTraits>;

public:
  OverlayTree();

  bool Frame(bool is3d);
  bool IsNeedUpdate() const;
  void ForceUpdate();

  void StartOverlayPlacing(ScreenBase const & screen);
  void Add(ref_ptr<OverlayHandle> handle);
  void EndOverlayPlacing();

  using TSelectResult = buffer_vector<ref_ptr<OverlayHandle>, 8>;
  void Select(m2::RectD const & rect, TSelectResult & result) const;
  void Select(m2::PointD const & glbPoint, TSelectResult & result) const;

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
                    detail::OverlayInfo const & parentOverlay);
  bool CheckHandle(ref_ptr<OverlayHandle> handle, int currentRank,
                   detail::OverlayInfo & parentOverlay) const;
  void AddHandleToDelete(detail::OverlayInfo const & overlay);

  int m_frameCounter;
  array<vector<ref_ptr<OverlayHandle>>, dp::OverlayRanksCount> m_handles;
  vector<detail::OverlayInfo> m_handlesToDelete;
  bool m_followingMode;

#ifdef COLLECT_DISPLACEMENT_INFO
  TDisplacementInfo m_displacementInfo;
#endif
};

} // namespace dp
