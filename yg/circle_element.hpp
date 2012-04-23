#pragma once

#include "overlay_element.hpp"
#include "circle_info.hpp"

namespace yg
{
  class CircleElement : public OverlayElement
  {
  private:

    yg::CircleInfo m_ci;

    mutable vector<m2::AnyRectD> m_boundRects;

    m2::AnyRectD const boundRect() const;

  public:

    typedef OverlayElement base_t;

    struct Params : public base_t::Params
    {
      yg::CircleInfo m_ci;
    };

    CircleElement(Params const & p);
    CircleElement(CircleElement const & ce, math::Matrix<double, 3, 3> const & m);

    vector<m2::AnyRectD> const & boundRects() const;

    void draw(gl::OverlayRenderer * s, math::Matrix<double, 3, 3> const & m) const;

    int visualRank() const;

    OverlayElement * clone(math::Matrix<double, 3, 3> const & m) const;
  };
}
