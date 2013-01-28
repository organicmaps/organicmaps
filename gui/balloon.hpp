#pragma once

#include "element.hpp"
#include "text_view.hpp"
#include "image_view.hpp"

#include "../graphics/font_desc.hpp"
#include "../graphics/image.hpp"

#include "../base/string_utils.hpp"
#include "../base/matrix.hpp"

#include "../std/scoped_ptr.hpp"
#include "../std/function.hpp"

namespace graphics
{
  class OverlayRenderer;
}

namespace gui
{
  class Balloon : public Element
  {
  private:

    typedef function<void (Element *)> TOnClickListener;

    mutable vector<m2::AnyRectD> m_boundRects;

    void cache();
    void purge();
    void layout();

    scoped_ptr<TextView> m_textView;
    scoped_ptr<ImageView> m_imageView;
    scoped_ptr<graphics::DisplayList> m_displayList;

    string m_text;
    graphics::Image::Info m_image;

    double m_textMarginLeft;
    double m_textMarginTop;
    double m_textMarginRight;
    double m_textMarginBottom;

    double m_imageMarginLeft;
    double m_imageMarginTop;
    double m_imageMarginRight;
    double m_imageMarginBottom;

    double m_arrowHeight;
    double m_arrowWidth;
    double m_arrowAngle;

    TOnClickListener m_onClickListener;

    typedef Element base_t;

  public:

    struct Params : public base_t::Params
    {
      string m_text;
      graphics::Image::Info m_image;
      double m_textMarginLeft;
      double m_textMarginTop;
      double m_textMarginRight;
      double m_textMarginBottom;
      double m_imageMarginLeft;
      double m_imageMarginTop;
      double m_imageMarginRight;
      double m_imageMarginBottom;
      Params();
    };

    Balloon(Params const & p);

    vector<m2::AnyRectD> const & boundRects() const;
    void draw(graphics::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;

    void setController(Controller * controller);
    void setPivot(m2::PointD const & pv);

    void setOnClickListener(TOnClickListener const & fn);

    void setText(string const & s);

    bool onTapStarted(m2::PointD const & pt);
    bool onTapMoved(m2::PointD const & pt);
    bool onTapEnded(m2::PointD const & pt);
    bool onTapCancelled(m2::PointD const & pt);
  };
}
