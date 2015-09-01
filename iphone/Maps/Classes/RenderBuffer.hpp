#pragma once

#include "RenderContext.hpp"
#include "std/shared_ptr.hpp"
#include "graphics/render_target.hpp"

@class CAEAGLLayer;

namespace iphone
{

class RenderBuffer : public graphics::RenderTarget
{
private:
  unsigned int m_id;
  shared_ptr<RenderContext> m_renderContext;
  int m_width;
  int m_height;

public:
  RenderBuffer(shared_ptr<RenderContext> renderContext, CAEAGLLayer * layer);
  RenderBuffer(shared_ptr<RenderContext> renderContext, int width, int height);
  ~RenderBuffer();

  void makeCurrent();

  unsigned int id() const;

  void present();

  unsigned width() const;
  unsigned height() const;

  void attachToFrameBuffer();
  void detachFromFrameBuffer();
  void coordMatrix(math::Matrix<float, 4, 4> & m);
};

}
