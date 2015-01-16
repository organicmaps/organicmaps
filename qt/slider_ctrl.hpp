#pragma once

#include <QtWidgets/QApplication>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  #include <QtGui/QSlider>
#else
  #include <QtWidgets/QSlider>
#endif

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
