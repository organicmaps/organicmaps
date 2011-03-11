#pragma once

#include "../base/assert.hpp"

#include "../map/window_handle.hpp"

#include <QtGui/QWidget>

namespace qt
{
  class WindowHandle : public ::WindowHandle
  {
    QWidget * m_pWnd;

  public:

    WindowHandle(QWidget * p) : m_pWnd(p) {}

    void invalidateImpl()
    {
      ASSERT ( m_pWnd != 0, () );
      m_pWnd->setUpdatesEnabled(true);
      m_pWnd->update();
    }
  };
}
