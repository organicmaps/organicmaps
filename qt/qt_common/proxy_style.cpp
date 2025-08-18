#include "qt/qt_common/proxy_style.hpp"

namespace qt
{
namespace common
{
ProxyStyle::ProxyStyle(QStyle * p) : QStyle(), style(p) {}

void ProxyStyle::drawComplexControl(ComplexControl control, QStyleOptionComplex const * option, QPainter * painter,
                                    QWidget const * widget) const
{
  style->drawComplexControl(control, option, painter, widget);
}

void ProxyStyle::drawControl(ControlElement element, QStyleOption const * option, QPainter * painter,
                             QWidget const * widget) const
{
  style->drawControl(element, option, painter, widget);
}

void ProxyStyle::drawItemPixmap(QPainter * painter, QRect const & rect, int alignment, QPixmap const & pixmap) const
{
  style->drawItemPixmap(painter, rect, alignment, pixmap);
}

void ProxyStyle::drawItemText(QPainter * painter, QRect const & rect, int alignment, QPalette const & pal, bool enabled,
                              QString const & text, QPalette::ColorRole textRole) const
{
  style->drawItemText(painter, rect, alignment, pal, enabled, text, textRole);
}

void ProxyStyle::drawPrimitive(PrimitiveElement elem, QStyleOption const * option, QPainter * painter,
                               QWidget const * widget) const
{
  style->drawPrimitive(elem, option, painter, widget);
}

QPixmap ProxyStyle::generatedIconPixmap(QIcon::Mode iconMode, QPixmap const & pixmap, QStyleOption const * option) const
{
  return style->generatedIconPixmap(iconMode, pixmap, option);
}

QStyle::SubControl ProxyStyle::hitTestComplexControl(ComplexControl control, QStyleOptionComplex const * option,
                                                     QPoint const & pos, QWidget const * widget) const
{
  return style->hitTestComplexControl(control, option, pos, widget);
}

QRect ProxyStyle::itemPixmapRect(QRect const & rect, int alignment, QPixmap const & pixmap) const
{
  return style->itemPixmapRect(rect, alignment, pixmap);
}

QRect ProxyStyle::itemTextRect(QFontMetrics const & metrics, QRect const & rect, int alignment, bool enabled,
                               QString const & text) const
{
  return style->itemTextRect(metrics, rect, alignment, enabled, text);
}

int ProxyStyle::pixelMetric(PixelMetric metric, QStyleOption const * option, QWidget const * widget) const
{
  return style->pixelMetric(metric, option, widget);
}

void ProxyStyle::polish(QWidget * widget)
{
  style->polish(widget);
}

void ProxyStyle::polish(QApplication * app)
{
  style->polish(app);
}

void ProxyStyle::polish(QPalette & pal)
{
  style->polish(pal);
}

QSize ProxyStyle::sizeFromContents(ContentsType type, QStyleOption const * option, QSize const & contentsSize,
                                   QWidget const * widget) const
{
  return style->sizeFromContents(type, option, contentsSize, widget);
}

QIcon ProxyStyle::standardIcon(StandardPixmap standardIcon, QStyleOption const * option, QWidget const * widget) const
{
  return style->standardIcon(standardIcon, option, widget);
}

QPalette ProxyStyle::standardPalette() const
{
  return style->standardPalette();
}

QPixmap ProxyStyle::standardPixmap(StandardPixmap standardPixmap, QStyleOption const * option,
                                   QWidget const * widget) const
{
  return style->standardPixmap(standardPixmap, option, widget);
}

int ProxyStyle::styleHint(StyleHint hint, QStyleOption const * option, QWidget const * widget,
                          QStyleHintReturn * returnData) const
{
  return style->styleHint(hint, option, widget, returnData);
}

QRect ProxyStyle::subControlRect(ComplexControl control, QStyleOptionComplex const * option, SubControl subControl,
                                 QWidget const * widget) const
{
  return style->subControlRect(control, option, subControl, widget);
}

QRect ProxyStyle::subElementRect(SubElement element, QStyleOption const * option, QWidget const * widget) const
{
  return style->subElementRect(element, option, widget);
}

void ProxyStyle::unpolish(QWidget * widget)
{
  style->unpolish(widget);
}

void ProxyStyle::unpolish(QApplication * app)
{
  style->unpolish(app);
}

int ProxyStyle::layoutSpacing(QSizePolicy::ControlType control1, QSizePolicy::ControlType control2,
                              Qt::Orientation orientation, QStyleOption const * option, QWidget const * widget) const
{
  return style->layoutSpacing(control1, control2, orientation, option, widget);
}
}  // namespace common
}  // namespace qt
