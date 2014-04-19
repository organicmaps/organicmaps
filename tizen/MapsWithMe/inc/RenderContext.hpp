#pragma once
#include "../../../graphics/opengl/gl_render_context.hpp"
#include <FUi.h>

namespace tizen
{
class RenderContext : public graphics::gl::RenderContext
{
public:
  RenderContext();
  virtual ~RenderContext();

  bool Init(::Tizen::Ui::Controls::Form * form);
  void SwapBuffers();
  virtual void makeCurrent();
  virtual RenderContext * createShared();
private:
  Tizen::Graphics::Opengl::EGLDisplay m_display;
  Tizen::Graphics::Opengl::EGLSurface m_surface;
  Tizen::Graphics::Opengl::EGLContext m_context;
};

}
