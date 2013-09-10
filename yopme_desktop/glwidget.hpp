#pragma once

#include <QtOpenGL/QGLWidget>

#include "../map/framework.hpp"
#include "../platform/video_timer.hpp"

class GLWidget : public QGLWidget
{
public:
  GLWidget(QWidget * parent);
  ~GLWidget();

protected:
  void initializeGL();
  void resizeGL(int w, int h);
  void paintGL();

private:
  Framework m_f;
  EmptyVideoTimer m_timer;
};
