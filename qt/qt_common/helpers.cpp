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
#include <QtGui/QSurfaceFormat>

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

void SetDefaultSurfaceFormat(QString const & platformName)
{
  QSurfaceFormat fmt;
  fmt.setAlphaBufferSize(8);
  fmt.setBlueBufferSize(8);
  fmt.setGreenBufferSize(8);
  fmt.setRedBufferSize(8);
  fmt.setStencilBufferSize(0);
  fmt.setSamples(0);
  fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
  fmt.setSwapInterval(1);
  fmt.setDepthBufferSize(16);
#if defined(OMIM_OS_LINUX)
  fmt.setRenderableType(QSurfaceFormat::OpenGLES);
  fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
#endif

  // Set proper OGL version now (needed for "cocoa" or "xcb"), but have troubles with "wayland" devices.
  // It will be resolved later in MapWidget::initializeGL when OGL context is available.
  if (platformName != QString("wayland"))
  {
#if defined(OMIM_OS_LINUX)
    LOG(LINFO, ("Set default OpenGL version to ES 3.0"));
    fmt.setVersion(3, 0);
#else
    LOG(LINFO, ("Set default OGL version to 3.2"));
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setVersion(3, 2);
#endif
  }

#if defined(ENABLE_OPENGL_DIAGNOSTICS)
  fmt.setOption(QSurfaceFormat::DebugContext);
#endif
  QSurfaceFormat::setDefaultFormat(fmt);
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
