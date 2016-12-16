#include "qt/slider_ctrl.hpp"
#include "qt/proxystyle.hpp"

#include "base/math.hpp"


namespace qt
{
  ///////////////////////////////////////////////////////////////////////////////////////////////
  // QClickSmoothSlider implementation
  ///////////////////////////////////////////////////////////////////////////////////////////////

  QClickSmoothSlider::QClickSmoothSlider(Qt::Orientation orient, QWidget * pParent, int factor)
   : base_t(orient, pParent), m_factor(factor)
  {
    /// This style cause slider to set value exactly to the cursor position (not "page scroll")
    /// @todo Do investigate this stuff with Qt5.
    class MyProxyStyle : public ProxyStyle
    {
    public:
      MyProxyStyle(QStyle * p) : ProxyStyle(p) {}

      virtual int styleHint(StyleHint hint, const QStyleOption* option, const QWidget* widget, QStyleHintReturn* returnData) const
      {
        if (hint == SH_Slider_AbsoluteSetButtons)
          return Qt::LeftButton;
        else
          return ProxyStyle::styleHint(hint, option, widget, returnData);
      }
    };

    setStyle(new MyProxyStyle(style()));
  }

  QClickSmoothSlider::~QClickSmoothSlider()
  {
    QStyle * p = style();
    delete p;
  }

  void QClickSmoothSlider::SetRange(int low, int up)
  {
    setRange(low * m_factor, up * m_factor);
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////
  // QScaleSlider implementation
  ///////////////////////////////////////////////////////////////////////////////////////////////

  QScaleSlider::QScaleSlider(Qt::Orientation orient, QWidget * pParent, int factor)
    : base_t(orient, pParent, factor)
  {
  }

  double QScaleSlider::GetScaleFactor() const
  {
    double const oldV = value();
    double const newV = sliderPosition();

    if (oldV != newV)
    {
      double const f = pow(2, fabs(oldV - newV) / m_factor);
      return (newV > oldV ? f : 1.0 / f);
    }
    else return 1.0;
  }

  void QScaleSlider::SetPosWithBlockedSignals(double pos)
  {
    bool const b = signalsBlocked();
    blockSignals(true);

    setSliderPosition(my::rounds(pos * m_factor));

    blockSignals(b);
  }
}
