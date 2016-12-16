#include "qt/proxystyle.hpp"


ProxyStyle::ProxyStyle(QStyle * p)
	: QStyle(), style(p)
{
}

void ProxyStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget) const
{
	style->drawComplexControl(control, option, painter, widget);
}

void ProxyStyle::drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget)  const
{
	style->drawControl(element, option, painter, widget);
}

void ProxyStyle::drawItemPixmap(QPainter* painter, const QRect& rect, int alignment, const QPixmap& pixmap) const
{
	style->drawItemPixmap(painter, rect, alignment, pixmap);
}

void ProxyStyle::drawItemText(QPainter* painter, const QRect& rect, int alignment, const QPalette& pal, bool enabled, const QString& text, QPalette::ColorRole textRole) const
{
	style->drawItemText(painter, rect, alignment, pal, enabled, text, textRole);
}

void ProxyStyle::drawPrimitive(PrimitiveElement elem, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
	style->drawPrimitive(elem, option, painter, widget);
}

QPixmap ProxyStyle::generatedIconPixmap(QIcon::Mode iconMode, const QPixmap& pixmap, const QStyleOption* option) const
{
	return style->generatedIconPixmap(iconMode, pixmap, option);
}

QStyle::SubControl ProxyStyle::hitTestComplexControl(ComplexControl control, const QStyleOptionComplex* option, const QPoint& pos, const QWidget* widget) const
{
	return style->hitTestComplexControl(control, option, pos, widget);
}

QRect ProxyStyle::itemPixmapRect(const QRect& rect, int alignment, const QPixmap& pixmap) const
{
	return style->itemPixmapRect(rect, alignment, pixmap);
}

QRect ProxyStyle::itemTextRect(const QFontMetrics& metrics, const QRect& rect, int alignment, bool enabled, const QString& text) const
{
	return style->itemTextRect(metrics, rect, alignment, enabled, text);
}

int ProxyStyle::pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const
{
	return style->pixelMetric(metric, option, widget);
}

void ProxyStyle::polish(QWidget* widget)
{
	style->polish(widget);
}

void ProxyStyle::polish(QApplication* app)
{
	style->polish(app);
}

void ProxyStyle::polish(QPalette& pal)
{
	style->polish(pal);
}

QSize ProxyStyle::sizeFromContents(ContentsType type, const QStyleOption* option, const QSize& contentsSize, const QWidget* widget) const
{
	return style->sizeFromContents(type, option, contentsSize, widget);
}

QIcon ProxyStyle::standardIcon(StandardPixmap standardIcon, const QStyleOption* option, const QWidget* widget) const
{
	return style->standardIcon(standardIcon, option, widget);
}

QPalette ProxyStyle::standardPalette() const
{
	return style->standardPalette();
}

QPixmap ProxyStyle::standardPixmap(StandardPixmap standardPixmap, const QStyleOption* option, const QWidget* widget) const
{
	return style->standardPixmap(standardPixmap, option, widget);
}

int ProxyStyle::styleHint(StyleHint hint, const QStyleOption* option, const QWidget* widget, QStyleHintReturn* returnData) const
{
	return style->styleHint(hint, option, widget, returnData);
}

QRect ProxyStyle::subControlRect(ComplexControl control, const QStyleOptionComplex* option, SubControl subControl, const QWidget* widget) const
{
	return style->subControlRect(control, option, subControl, widget);
}

QRect ProxyStyle::subElementRect(SubElement element, const QStyleOption* option, const QWidget* widget) const
{
	return style->subElementRect(element, option, widget);
}

void ProxyStyle::unpolish(QWidget* widget)
{
	style->unpolish(widget);
}

void ProxyStyle::unpolish(QApplication* app)
{
	style->unpolish(app);
}

int ProxyStyle::layoutSpacing(QSizePolicy::ControlType control1,
                              QSizePolicy::ControlType control2, Qt::Orientation orientation,
                              const QStyleOption *option, const QWidget *widget) const
{
  return style->layoutSpacing(control1, control2, orientation, option, widget);
}
