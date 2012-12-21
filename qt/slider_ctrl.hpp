#pragma once

#include <QtWidgets/QSlider>

namespace qt
{
  class QClickSmoothSlider : public QSlider
  {
    typedef QSlider base_t;

  protected:
    int m_factor;

  public:
    QClickSmoothSlider(Qt::Orientation orient, QWidget * pParent, int factor);
    virtual ~QClickSmoothSlider();

    void SetRange(int low, int up);
  };

  class QScaleSlider : public QClickSmoothSlider
  {
    typedef QClickSmoothSlider base_t;

  public:
    QScaleSlider(Qt::Orientation orient, QWidget * pParent, int factor);

    double GetScaleFactor() const;
    void SetPosWithBlockedSignals(double pos);
  };
}
