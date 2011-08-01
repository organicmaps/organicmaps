#pragma once

#include "widgets.hpp"

namespace qt
{
  template <class T>
  GLDrawWidgetT<T>::~GLDrawWidgetT()
  {}

  template <class T>
  void GLDrawWidgetT<T>::paintGL()
  {
    if (m_p)
      this->DoDraw(m_p);
  }

  template <class T>
  void GLDrawWidgetT<T>::resizeGL(int w, int h)
  {
    if (m_p)
    {
      m_p->onSize(w, h);
      this->DoResize(w, h);
    }
  }
}
