#pragma once

#include <QMouseEvent>

#include "drape_frontend/user_event_stream.hpp"

namespace qt::common::renderer::base
{
bool IsLeftButton(Qt::MouseButtons buttons);
bool IsLeftButton(QMouseEvent const * e);

bool IsRightButton(Qt::MouseButtons buttons);
bool IsRightButton(QMouseEvent const * e);

bool IsCommandModifier(QMouseEvent const * e);
bool IsShiftModifier(QMouseEvent const * e);
bool IsAltModifier(QMouseEvent const * e);
}  // namespace qt::common::renderer::base
