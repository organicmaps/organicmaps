#pragma once

#include "drape/overlay_handle.hpp"

#include "geometry/screenbase.hpp"
#include "geometry/tree4d.hpp"


namespace dp
{

namespace detail
{

struct OverlayTraits
{
  ScreenBase m_modelView;

  inline m2::RectD const LimitRect(ref_ptr<OverlayHandle> handle)
  {
    return handle->GetPixelRect(m_modelView);
  }
};

}

class OverlayTree : public m4::Tree<ref_ptr<OverlayHandle>, detail::OverlayTraits>
{
  typedef m4::Tree<ref_ptr<OverlayHandle>, detail::OverlayTraits> BaseT;

public:
  void StartOverlayPlacing(ScreenBase const & screen, bool canOverlap = false);
  void Add(ref_ptr<OverlayHandle> handle);
  void EndOverlayPlacing();

private:
  ScreenBase const & GetModelView() const { return m_traits.m_modelView; }

private:
  bool m_canOverlap;
};

} // namespace dp
