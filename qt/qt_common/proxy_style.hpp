#pragma once

#include <QtWidgets/QStyle>

namespace qt
{
namespace common
{
class ProxyStyle : public QStyle
{
public:
  explicit ProxyStyle(QStyle * p);

  // QStyle overrides:
  void drawComplexControl(ComplexControl control, QStyleOptionComplex const * option, QPainter * painter,
                          QWidget const * widget = 0) const override;
  void drawControl(ControlElement element, QStyleOption const * option, QPainter * painter,
                   QWidget const * widget = 0) const override;
  void drawItemPixmap(QPainter * painter, QRect const & rect, int alignment, QPixmap const & pixmap) const override;
  void drawItemText(QPainter * painter, QRect const & rect, int alignment, QPalette const & pal, bool enabled,
                    QString const & text, QPalette::ColorRole textRole = QPalette::NoRole) const override;
  void drawPrimitive(PrimitiveElement elem, QStyleOption const * option, QPainter * painter,
                     QWidget const * widget = 0) const override;
  QPixmap generatedIconPixmap(QIcon::Mode iconMode, QPixmap const & pixmap, QStyleOption const * option) const override;
  SubControl hitTestComplexControl(ComplexControl control, QStyleOptionComplex const * option, QPoint const & pos,
                                   QWidget const * widget = 0) const override;
  QRect itemPixmapRect(QRect const & rect, int alignment, QPixmap const & pixmap) const override;
  QRect itemTextRect(QFontMetrics const & metrics, QRect const & rect, int alignment, bool enabled,
                     QString const & text) const override;
  int pixelMetric(PixelMetric metric, QStyleOption const * option = 0, QWidget const * widget = 0) const override;
  void polish(QWidget * widget) override;
  void polish(QApplication * app) override;
  void polish(QPalette & pal) override;
  QSize sizeFromContents(ContentsType type, QStyleOption const * option, QSize const & contentsSize,
                         QWidget const * widget = 0) const override;
  QIcon standardIcon(StandardPixmap standardIcon, QStyleOption const * option = 0,
                     QWidget const * widget = 0) const override;
  QPalette standardPalette() const override;
  QPixmap standardPixmap(StandardPixmap standardPixmap, QStyleOption const * option = 0,
                         QWidget const * widget = 0) const override;
  int styleHint(StyleHint hint, QStyleOption const * option = 0, QWidget const * widget = 0,
                QStyleHintReturn * returnData = 0) const override;
  QRect subControlRect(ComplexControl control, QStyleOptionComplex const * option, SubControl subControl,
                       QWidget const * widget = 0) const override;
  QRect subElementRect(SubElement element, QStyleOption const * option, QWidget const * widget = 0) const override;
  void unpolish(QWidget * widget) override;
  void unpolish(QApplication * app) override;
  int layoutSpacing(QSizePolicy::ControlType control1, QSizePolicy::ControlType control2, Qt::Orientation orientation,
                    QStyleOption const * option = 0, QWidget const * widget = 0) const override;

private:
  QStyle * style;
};
}  // namespace common
}  // namespace qt
