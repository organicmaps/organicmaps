/*
 *
 * window_handle.cpp
 *
 *  Created on: 26 Aug 2010
 *      Author: Alex
 */

#include "window_handle.hpp"

namespace bada
{
  WindowHandle::WindowHandle(MapsForm * form) : m_form(form)
  {
  }

  void WindowHandle::invalidate()
  {
    m_form->RequestRedraw();
  }
}
