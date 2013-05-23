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
  class Screen;
}

namespace gui
{
  class Balloon : public Element
  {
  protected:
    scoped_ptr<TextView> m_mainTextView;
    scoped_ptr<TextView> m_auxTextView;
    scoped_ptr<ImageView> m_imageView;

    double m_balloonScale;
    double m_textImageOffsetH;
    double m_textImageOffsetV;

  private:
    enum ETextMode
    {
      NoText = 0,
      SingleMainText = 0x1,
      SingleAuxText = 0x2,
      DualText = SingleMainText | SingleAuxText
    };

    ETextMode m_textMode;

    uint32_t m_maxWidth;

    void updateTextMode(string const & main, string const & aux);

    scoped_ptr<graphics::DisplayList> m_displayList;

    typedef function<void (Element *)> TOnClickListener;

    mutable vector<m2::AnyRectD> m_boundRects;

    void cacheBorders(graphics::Screen * cs,
                      graphics::Image::Info & borderImg,
                      uint32_t balloonWidth,
                      uint32_t arrowHeight);
    void cacheBody(graphics::Screen * cs,
                   graphics::Image::Info & bodyImg,
                   double offsetX,
                   double bodyWidth,
                   uint32_t arrowHeight);

    void initBgImages();

    void cache();
    void purge();
    void layout();

    void layoutMainText(double balloonWidth,
                        double leftMargin);
    void layoutAuxText(double balloonWidth,
                       double leftMargin);

    void calcMaxTextWidth();

    graphics::Image::Info m_borderImg;
    graphics::Image::Info m_bodyImg;
    graphics::Image::Info m_arrowImg;

    TOnClickListener m_onClickListener;

    typedef Element base_t;

  public:

    struct Params : public base_t::Params
    {
      string m_mainText;
      string m_auxText;
      graphics::Image::Info m_image;
      Params();
    };

    Balloon(Params const & p);

    vector<m2::AnyRectD> const & boundRects() const;
    void draw(graphics::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;

    void setController(Controller * controller);
    void setPivot(m2::PointD const & pv);

    void setOnClickListener(TOnClickListener const & fn);

    void setText(string const & main, string const & aux);
    void setImage(graphics::Image::Info const & info);

    bool onTapStarted(m2::PointD const & pt);
    bool onTapMoved(m2::PointD const & pt);
    bool onTapEnded(m2::PointD const & pt);
    bool onTapCancelled(m2::PointD const & pt);

    void onScreenSize(int w, int h);
  };
}
