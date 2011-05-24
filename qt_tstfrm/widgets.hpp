#pragma once

#include <QtOpenGL/qgl.h>

#include "../map/drawer_yg.hpp"

#include "../std/shared_ptr.hpp"

namespace qt
{
  template <class TScreen, class TBase> class BaseDrawWidget : public TBase
  {
    typedef TBase base_type;

  public:
    typedef TScreen screen_t;

    BaseDrawWidget(QWidget * pParent) : base_type(pParent)
    {
    }

  protected:
    /// Override this function to make drawing and additional resize processing.
    //@{
    virtual void DoDraw(shared_ptr<screen_t> p) = 0;
    virtual void DoResize(int w, int h) = 0;
    //@}
  };

  /// Widget uses yg for drawing.
  template <class T> class GLDrawWidgetT : public BaseDrawWidget<T, QGLWidget>
  {
    typedef BaseDrawWidget<T, QGLWidget> base_type;
  protected:

    shared_ptr<T> m_p;

  public:

    shared_ptr<T> GetDrawer() {return m_p;}

    GLDrawWidgetT(QWidget * pParent) : base_type(pParent){}
    virtual ~GLDrawWidgetT();

  protected:
    /// Overriden from QGLWidget.
    //@{
    virtual void initializeGL() = 0;
    virtual void paintGL();
    virtual void resizeGL(int w, int h);
    //@}
  };
}
