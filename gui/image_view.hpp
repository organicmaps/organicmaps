#pragma once

#include "element.hpp"

#include "../graphics/image.hpp"
#include "../graphics/display_list.hpp"

#include "../std/unique_ptr.hpp"

namespace graphics
{
  class OverlayRenderer;
}

namespace gui
{
  class ImageView : public Element
  {
  private:

    mutable vector<m2::AnyRectD> m_boundRects;

    graphics::Image::Info m_image;
    m2::RectU m_margin;
    unique_ptr<graphics::DisplayList> m_displayList;

  public:

    void cache();
    void purge();

    typedef Element base_t;

    struct Params : public base_t::Params
    {
      graphics::Image::Info m_image;
      Params();
    };

    ImageView(Params const & p);

    vector<m2::AnyRectD> const & boundRects() const;

    void draw(graphics::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;
    void setImage(graphics::Image::Info const & info);
  };
}
