#include "qt/qt_common/helpers.hpp"

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
}  // namespace common
}  // namespace qt
