#include "../base/SRC_FIRST.hpp"

#include "slider_ctrl.hpp"
#include "proxystyle.hpp"

#include "../base/start_mem_debug.hpp"


namespace qt
{
  QClickSlider::QClickSlider(Qt::Orientation orient, QWidget * pParent)
   : QSlider(orient, pParent)
  {
    // this style cause slider to set value exactly to the cursor position (not "page scroll")
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

  QClickSlider::~QClickSlider()
  {
    QStyle * p = style();
    delete p;
  }
}
