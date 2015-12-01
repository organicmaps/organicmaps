#pragma once

#include "drape/overlay_handle.hpp"

#include "geometry/screenbase.hpp"
#include "geometry/tree4d.hpp"

#include "base/buffer_vector.hpp"

#include "std/array.hpp"
#include "std/vector.hpp"

namespace dp
{

namespace detail
{

struct OverlayInfo
{
  ref_ptr<OverlayHandle> m_handle;
  bool m_isTransparent = false;

  OverlayInfo() = default;
  OverlayInfo(ref_ptr<OverlayHandle> handle, bool isTransparent)
    : m_handle(handle)
    , m_isTransparent(isTransparent)
  {}

  bool operator==(OverlayInfo const & rhs) const
  {
    return m_handle == rhs.m_handle && m_isTransparent == rhs.m_isTransparent;
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

  bool Frame();
  bool IsNeedUpdate() const;
  void ForceUpdate();

  void StartOverlayPlacing(ScreenBase const & screen);
  void Add(ref_ptr<OverlayHandle> handle, bool isTransparent);
  void EndOverlayPlacing();

  using TSelectResult = buffer_vector<ref_ptr<OverlayHandle>, 8>;
  void Select(m2::RectD const & rect, TSelectResult & result) const;

  using THandle = pair<ref_ptr<OverlayHandle>, bool>;

private:
  ScreenBase const & GetModelView() const { return m_traits.m_modelView; }
  void InsertHandle(ref_ptr<OverlayHandle> handle, bool isTransparent,
                    detail::OverlayInfo const & parentOverlay);
  bool CheckHandle(ref_ptr<OverlayHandle> handle, int currentRank,
                   detail::OverlayInfo & parentOverlay) const;
  void AddHandleToDelete(detail::OverlayInfo const & overlay);

  int m_frameCounter;
  array<vector<THandle>, dp::OverlayRanksCount> m_handles;
  vector<detail::OverlayInfo> m_handlesToDelete;
};

} // namespace dp
