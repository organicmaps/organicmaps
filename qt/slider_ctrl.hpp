#pragma once

#include <QtGui/QSlider>

namespace qt
{
  class QClickSlider : public QSlider
  {
  public:
    QClickSlider(Qt::Orientation orient, QWidget * pParent);
    virtual ~QClickSlider();
  };
}
