#pragma once

#include "platform/location.hpp"

#include <QtCore/QString>
#include <QtCore/Qt>
#include <QtGui/QKeySequence>
#include <QtGui/QMouseEvent>

class Framework;
class QIcon;

namespace qt::common
{
bool IsLeftButton(Qt::MouseButtons buttons);
bool IsLeftButton(QMouseEvent const * const e);

bool IsRightButton(Qt::MouseButtons buttons);
bool IsRightButton(QMouseEvent const * const e);

bool IsCommandModifier(QMouseEvent const * const e);
bool IsShiftModifier(QMouseEvent const * const e);
bool IsAltModifier(QMouseEvent const * const e);

struct Hotkey
{
  Hotkey() = default;
  Hotkey(QKeySequence const & key, char const * slot) : m_key(key), m_slot(slot) {}

  QKeySequence m_key = 0;
  char const * m_slot = nullptr;
};

location::GpsInfo MakeGpsInfo(m2::PointD const & point);

void SetDefaultSurfaceFormat(QString const & platformName);

bool IsSystemInDarkMode();
void ApplySystemNightMode(Framework & framework);

// Returns the appropriate toolbar icon for the current system theme.
// |lightPath| is a resource path like ":/navig64/search.png".
// The dark variant is expected under the same path with "-dark" prefix
// (e.g. ":/navig64-dark/search.png").
QIcon GetToolbarIcon(char const * lightPath);
}  // namespace qt::common
