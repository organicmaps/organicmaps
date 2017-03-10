#pragma once

#include <QtCore/Qt>
#include <QtWidgets/QSlider>

class QToolBar;

namespace qt
{
namespace common
{
class MapWidget;

class ScaleSlider : public QSlider
{
public:
  ScaleSlider(Qt::Orientation orient, QWidget * parent);

  static void Embed(Qt::Orientation orient, QToolBar & toolBar, MapWidget & mapWidget);

  double GetScaleFactor() const;
  void SetPosWithBlockedSignals(double pos);

private:
  void SetRange(int low, int up);

  int m_factor;
};
}  // namespace common
}  // namespace qt
