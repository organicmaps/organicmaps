#include "openlr/openlr_match_quality/openlr_assessment_tool/map_widget.hpp"

#include "map/framework.hpp"

MapWidget::MapWidget(Framework & framework, bool apiOpenGLES3, QWidget * parent)
  : Base(framework, apiOpenGLES3, parent)
{
}

MapWidget::~MapWidget() {}

void MapWidget::mousePressEvent(QMouseEvent * e)
{
  Base::mousePressEvent(e);

  if (m_mode == Mode::TrafficMarkup)
  {
    auto pt = GetDevicePoint(e);
    emit TrafficMarkupClick(m_framework.PtoG(pt));
  }
}
