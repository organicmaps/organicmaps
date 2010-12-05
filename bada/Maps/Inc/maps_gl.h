#pragma once

#include "../../../base/SRC_FIRST.hpp"

#include <FGraphicsOpengl.h>

using namespace Osp::Graphics::Opengl;

#include "../../../base/ptr_utils.hpp"
#include "../../../std/shared_ptr.hpp"
#include "../../../yg/framebuffer.hpp"
#include "../../../yg/renderbuffer.hpp"
#include "../../../map/drawer_yg.hpp"
#include "../../../map/framework.hpp"
#include "../../../map/render_queue.hpp"
#include "../../../map/navigator.hpp"
#include "../../../map/feature_vec_model.hpp"
#include "window_handle.hpp"

class MapsControl;
class MapsForm;
namespace bada
{
  struct RenderContext;
}

/// OpenGl wrapper for bada
class MapsGl
{
  MapsControl * __pApplication;
  MapsForm * __pForm;

  typedef RenderQueue<DrawerYG, bada::RenderContext, bada::WindowHandle> render_queue_t;
  typedef FrameWork<model::FeaturesFetcher, Navigator, bada::WindowHandle, render_queue_t> framework_t;

  shared_ptr<bada::RenderContext> m_primaryContext;
  shared_ptr<DrawerYG> m_drawer;
  shared_ptr<yg::gl::FrameBuffer> m_frameBuffer;
  shared_ptr<yg::gl::RenderBuffer> m_depthBuffer;
  shared_ptr<bada::WindowHandle> m_windowHandle;

  shared_ptr<framework_t> m_framework;

public:
  MapsGl(MapsControl * app, MapsForm * form);
  ~MapsGl();

  void Draw();

  bool Init();

  framework_t * Framework() { return m_framework.get(); }
};
