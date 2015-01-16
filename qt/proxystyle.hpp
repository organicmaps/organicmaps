#pragma once

#include <QtWidgets/QApplication>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  #include <QtGui/QStyle>
#else
  #include <QtWidgets/QStyle>
#endif


class ProxyStyle : public QStyle
{
public:
	explicit ProxyStyle(QStyle * p);

	void drawComplexControl(ComplexControl control, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget = 0) const;
	void drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = 0)  const;
	void drawItemPixmap(QPainter* painter, const QRect& rect, int alignment, const QPixmap& pixmap) const;
	void drawItemText(QPainter* painter, const QRect& rect, int alignment, const QPalette& pal, bool enabled, const QString& text, QPalette::ColorRole textRole = QPalette::NoRole) const;
	void drawPrimitive(PrimitiveElement elem, const QStyleOption* option, QPainter* painter, const QWidget* widget = 0) const;
	QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap& pixmap, const QStyleOption* option) const;
	SubControl hitTestComplexControl(ComplexControl control, const QStyleOptionComplex* option, const QPoint& pos, const QWidget* widget = 0) const;
	QRect itemPixmapRect(const QRect& rect, int alignment, const QPixmap& pixmap) const;
	QRect itemTextRect(const QFontMetrics& metrics, const QRect& rect, int alignment, bool enabled, const QString& text) const;
	int pixelMetric(PixelMetric metric, const QStyleOption* option = 0, const QWidget* widget = 0) const;
	void polish(QWidget* widget);
	void polish(QApplication* app);
	void polish(QPalette& pal);
	QSize sizeFromContents(ContentsType type, const QStyleOption* option, const QSize& contentsSize, const QWidget* widget = 0) const;
	QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption* option = 0, const QWidget* widget = 0) const;
	QPalette standardPalette() const;
	QPixmap standardPixmap(StandardPixmap standardPixmap, const QStyleOption* option = 0, const QWidget* widget = 0) const;
	int styleHint(StyleHint hint, const QStyleOption* option = 0, const QWidget* widget = 0, QStyleHintReturn* returnData = 0) const;
	QRect subControlRect(ComplexControl control, const QStyleOptionComplex* option, SubControl subControl, const QWidget* widget = 0) const;
	QRect subElementRect(SubElement element, const QStyleOption* option, const QWidget* widget = 0) const;
	void unpolish(QWidget* widget);
	void unpolish(QApplication* app);
  int layoutSpacing(QSizePolicy::ControlType control1,
                    QSizePolicy::ControlType control2, Qt::Orientation orientation,
                    const QStyleOption *option = 0, const QWidget *widget = 0) const;

private:
	QStyle * style;
};
