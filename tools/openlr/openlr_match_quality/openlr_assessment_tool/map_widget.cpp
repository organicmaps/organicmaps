#include "openlr/openlr_match_quality/openlr_assessment_tool/map_widget.hpp"

#include "qt/qt_common/helpers.hpp"

#include "map/framework.hpp"

#include <QMouseEvent>

namespace openlr
{
MapWidget::MapWidget(Framework & framework, QWidget * parent) : Base(framework, false /* screenshotMode */, parent) {}

void MapWidget::mousePressEvent(QMouseEvent * e)
{
  Base::mousePressEvent(e);

  if (qt::common::IsRightButton(e))
    ShowInfoPopup(e, GetDevicePoint(e));

  if (m_mode == Mode::TrafficMarkup)
  {
    auto pt = GetDevicePoint(e);
    emit TrafficMarkupClick(m_framework.PtoG(pt), e->button());
  }
}
}  // namespace openlr
