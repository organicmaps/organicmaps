#include "qt/qt_common/helpers.hpp"

#include "geometry/mercator.hpp"

#include <QtCore/QDateTime>

namespace qt
{
namespace common
{
bool IsLeftButton(Qt::MouseButtons buttons) { return buttons & Qt::LeftButton; }

bool IsLeftButton(QMouseEvent const * const e)
{
  return IsLeftButton(e->button()) || IsLeftButton(e->buttons());
}

bool IsRightButton(Qt::MouseButtons buttons) { return buttons & Qt::RightButton; }

bool IsRightButton(QMouseEvent const * const e)
{
  return IsRightButton(e->button()) || IsRightButton(e->buttons());
}

bool IsCommandModifier(QMouseEvent const * const e) { return e->modifiers() & Qt::ControlModifier; }

bool IsShiftModifier(QMouseEvent const * const e) { return e->modifiers() & Qt::ShiftModifier; }

bool IsAltModifier(QMouseEvent const * const e) { return e->modifiers() & Qt::AltModifier; }

location::GpsInfo MakeGpsInfo(m2::PointD const & point)
{
  location::GpsInfo info;
  info.m_source = location::EUser;
  info.m_latitude = MercatorBounds::YToLat(point.y);
  info.m_longitude = MercatorBounds::XToLon(point.x);
  info.m_horizontalAccuracy = 10;
  info.m_timestamp = QDateTime::currentMSecsSinceEpoch() / 1000.0;
  return info;
}
}  // namespace common
}  // namespace qt
