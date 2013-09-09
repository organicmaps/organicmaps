#pragma once

#include <QtOpenGL/QGLWidget>

#include "../map/framework.hpp"

class GLWidget : public QGLWidget
{
public:
  GLWidget(QWidget * parent);

protected:
  void initializeGL();
  void resizeGL(int w, int h);
  void paintGL();

private:
  Framework m_f;
};
