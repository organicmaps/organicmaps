#pragma once

#include "overlay_handle.hpp"

#include "../geometry/screenbase.hpp"
#include "../geometry/tree4d.hpp"


namespace dp
{

namespace detail
{

struct OverlayTraits
{
  ScreenBase m_modelView;

  inline m2::RectD const LimitRect(RefPointer<OverlayHandle> handle)
  {
    return handle->GetPixelRect(m_modelView);
  }
};

}

class OverlayTree : public m4::Tree<RefPointer<OverlayHandle>, detail::OverlayTraits>
{
  typedef m4::Tree<RefPointer<OverlayHandle>, detail::OverlayTraits> BaseT;

public:
  void StartOverlayPlacing(ScreenBase const & screen, bool canOverlap = false);
  void Add(RefPointer<OverlayHandle> handle);
  void EndOverlayPlacing();

private:
  ScreenBase const & GetModelView() const { return m_traits.m_modelView; }

private:
  bool m_canOverlap;
};

} // namespace dp
