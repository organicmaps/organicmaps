#include "render_context.hpp"
#include "../map_server_utils/viewport.hpp"
#include "../map_server_utils/request.hpp"
#include "../map_server_utils/response.hpp"

#include "../graphics/resource_manager.hpp"

#include "../map/render_policy.hpp"
#include "../map/simple_render_policy.hpp"
#include "../map/framework.hpp"

#include "../gui/controller.hpp"

#include "../platform/platform.hpp"


#include "../std/shared_ptr.hpp"

#include <QGLPixelBuffer>
#include <QtCore/QBuffer>
#include <QtGui/QApplication>

namespace
{
  void empty()
  {}

  class EmptyVideoTimer : public VideoTimer
  {
  public:
    EmptyVideoTimer()
      : VideoTimer(bind(&empty))
    {}

    ~EmptyVideoTimer()
    {
      stop();
    }

    void start()
    {
      if (m_state == EStopped)
        m_state = ERunning;
    }

    void resume()
    {
      if (m_state == EPaused)
      {
        m_state = EStopped;
        start();
      }
    }

    void pause()
    {
      stop();
      m_state = EPaused;
    }

    void stop()
    {
      if (m_state == ERunning)
        m_state = EStopped;
    }

    void perform()
    {}
  };
}

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  srv::Request request = srv::GetRequest(cin);
  srv::Viewport v = request.GetViewport();

  QGLPixelBuffer * pb = new QGLPixelBuffer(QSize(v.GetWidth(), v.GetHeight()));
  pb->makeCurrent();

  Framework framework;
  //framework.XorQueryMaxScaleMode();

  shared_ptr<srv::RenderContext> primaryRC(new srv::RenderContext());

  graphics::ResourceManager::Params rmParams;
  rmParams.m_rtFormat = graphics::Data8Bpp;
  rmParams.m_texFormat = graphics::Data8Bpp;
  rmParams.m_texRtFormat = graphics::Data4Bpp;
  rmParams.m_videoMemoryLimit = GetPlatform().VideoMemoryLimit();

  RenderPolicy::Params rpParams;

  shared_ptr<VideoTimer> timer(new EmptyVideoTimer());

  rpParams.m_videoTimer = timer.get();
  rpParams.m_useDefaultFB = true;
  rpParams.m_rmParams = rmParams;
  rpParams.m_primaryRC = primaryRC;
  rpParams.m_density = request.GetDensity();
  rpParams.m_skinName = "basic.skn";
  rpParams.m_screenWidth = v.GetWidth();
  rpParams.m_screenHeight = v.GetHeight();

  try
  {
    framework.SetRenderPolicy(new SimpleRenderPolicy(rpParams));
    framework.GetGuiController()->ResetRenderParams();
  }
  catch (graphics::gl::platform_unsupported const & e)
  {
    LOG(LERROR, ("OpenGL platform is unsupported, reason: ", e.what()));
  }

  framework.OnSize(v.GetWidth(), v.GetHeight());
  framework.GetNavigator().SetFromRect(v.GetAnyRect());

  shared_ptr<PaintEvent> pe(new PaintEvent(framework.GetRenderPolicy()->GetDrawer().get()));

  framework.BeginPaint(pe);
  framework.DoPaint(pe);
  framework.EndPaint(pe);

  pb->doneCurrent();

  QByteArray ba;
  QBuffer b(&ba);
  b.open(QIODevice::WriteOnly);

  pb->toImage().save(&b, "PNG");

  pb->makeCurrent();
  framework.SetRenderPolicy(0);
  pb->doneCurrent();

  srv::Response * response = request.CreateResponse(ba);
  response->DoResponse();
  delete response;
}
