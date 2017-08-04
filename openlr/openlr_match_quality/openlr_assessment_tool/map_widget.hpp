#pragma once

#include "qt/qt_common/map_widget.hpp"

namespace
{
class PointsController;
}  // namespace

class Framework;

class MapWidget : public qt::common::MapWidget
{
  Q_OBJECT

  using Base = qt::common::MapWidget;

  enum class Mode
  {
    Normal,
    TrafficMarkup
  };

public:
  MapWidget(Framework & framework, bool apiOpenGLES3, QWidget * parent);
  ~MapWidget() override = default;

  void SetNormalMode() { m_mode = Mode::Normal; }
  void SetTrafficMarkupMode() { m_mode = Mode::TrafficMarkup; }

signals:
  void TrafficMarkupClick(m2::PointD const & p, Qt::MouseButton const b);

protected:
  void mousePressEvent(QMouseEvent * e) override;

private:
  Mode m_mode = Mode::Normal;
};
