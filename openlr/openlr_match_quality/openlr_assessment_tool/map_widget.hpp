#pragma once

#include "qt/qt_common/map_widget.hpp"

namespace
{
class PointsController;
}  // namespace

class Framework;

namespace openlr
{
class MapWidget : public qt::common::MapWidget
{
  Q_OBJECT

  using Base = qt::common::MapWidget;

public:
  enum class Mode
  {
    Normal,
    TrafficMarkup
  };

  MapWidget(Framework & framework, QWidget * parent);
  ~MapWidget() override = default;

  void SetMode(Mode const mode) { m_mode = mode; }

  QSize sizeHint() const override { return QSize(800, 600); }

signals:
  void TrafficMarkupClick(m2::PointD const & p, Qt::MouseButton const b);

protected:
  void mousePressEvent(QMouseEvent * e) override;

private:
  Mode m_mode = Mode::Normal;
};
}  // namespace openlr
