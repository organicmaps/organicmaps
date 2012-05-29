#pragma once

#include "element.hpp"

#include "../std/function.hpp"
#include "../std/string.hpp"

namespace yg
{
  class OverlayElement;

  namespace gl
  {
    class OverlayRenderer;
  }
}

namespace gui
{
  class Button : public Element
  {
  public:

    typedef function<void (Element const *)> TOnClickListener;

  private:

    TOnClickListener m_OnClickListener;

    unsigned m_widthInDIP;
    unsigned m_heightInDIP;

    string m_text;
    mutable vector<m2::AnyRectD> m_boundRects;

  public:

    struct Params : Element::Params
    {
      unsigned m_width;
      unsigned m_height;
      string m_text;
    };

    Button(Params const & p);

    bool onTapStarted(m2::PointD const & pt);
    bool onTapMoved(m2::PointD const & pt);
    bool onTapEnded(m2::PointD const & pt);
    bool onTapCancelled(m2::PointD const & pt);

    void setOnClickListener(TOnClickListener const & l);

    void setText(string const & text);
    string const & text() const;

    void setWidth(unsigned widthInDIP);
    unsigned width() const;

    void setHeight(unsigned heightInDIP);
    unsigned height() const;

    /// Inherited from OverlayElement
    /// @{

    yg::OverlayElement * clone(math::Matrix<double, 3, 3> const & m) const;
    vector<m2::AnyRectD> const & boundRects() const;
    void draw(yg::gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;
    int visualRank() const;

    /// @}
  };
}
