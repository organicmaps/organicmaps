#include "qt/qt_common/helpers.hpp"

#include "geometry/mercator.hpp"

#include "map/framework.hpp"
#include "platform/location.hpp"
#include "platform/style_utils.hpp"

#include "indexer/map_style.hpp"

#include "base/logging.hpp"

#include <QtCore/QDateTime>
#include <QtCore/Qt>
#include <QtGui/QGuiApplication>
#include <QtGui/QMouseEvent>
#include <QtGui/QPalette>
#include <QtGui/QStyleHints>

namespace qt::common
{

bool IsLeftButton(Qt::MouseButtons buttons)
{
  return buttons & Qt::LeftButton;
}

bool IsLeftButton(QMouseEvent const * const e)
{
  return IsLeftButton(e->button()) || IsLeftButton(e->buttons());
}

bool IsRightButton(Qt::MouseButtons buttons)
{
  return buttons & Qt::RightButton;
}

bool IsRightButton(QMouseEvent const * const e)
{
  return IsRightButton(e->button()) || IsRightButton(e->buttons());
}

bool IsCommandModifier(QMouseEvent const * const e)
{
  return e->modifiers() & Qt::ControlModifier;
}

bool IsShiftModifier(QMouseEvent const * const e)
{
  return e->modifiers() & Qt::ShiftModifier;
}

bool IsAltModifier(QMouseEvent const * const e)
{
  return e->modifiers() & Qt::AltModifier;
}

location::GpsInfo MakeGpsInfo(m2::PointD const & point)
{
  location::GpsInfo info;
  info.m_source = location::EUser;
  info.m_latitude = mercator::YToLat(point.y);
  info.m_longitude = mercator::XToLon(point.x);
  info.m_horizontalAccuracy = 10;
  info.m_timestamp = QDateTime::currentMSecsSinceEpoch() / 1000.0;
  return info;
}

bool IsSystemInDarkMode()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
  if (auto * hints = QGuiApplication::styleHints(); hints != nullptr)
  {
    switch (hints->colorScheme())
    {
    case Qt::ColorScheme::Dark: return true;
    case Qt::ColorScheme::Light: return false;
    default: break;
    }
  }
#endif

  auto const palette = QGuiApplication::palette();
  auto const windowColor = palette.color(QPalette::Window);
  auto const textColor = palette.color(QPalette::WindowText);
  return windowColor.value() < textColor.value();
}

void ApplySystemNightMode(Framework & framework)
{
  if (style_utils::GetNightModeSetting() != style_utils::NightMode::System)
    return;

  auto const currentStyle = framework.GetMapStyle();
  auto const useDark = IsSystemInDarkMode();
  auto const desiredStyle = useDark ? GetDarkMapStyleVariant(currentStyle) : GetLightMapStyleVariant(currentStyle);
  if (desiredStyle != currentStyle)
    framework.SetMapStyle(desiredStyle);
}

}  // namespace qt::common
