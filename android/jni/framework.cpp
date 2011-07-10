#include "framework.h"
#include "jni_helper.h"
#include "rendering.h"
#include "../../std/shared_ptr.hpp"

#include "../../base/logging.hpp"


void AndroidFramework::ViewHandle::invalidateImpl()
{
  m_parent->CallRepaint();
}

void AndroidFramework::CallRepaint()
{
  m_env->CallVoidMethod(m_parentView,
          jni::GetJavaMethodID(m_env, m_parentView, "requestRender", "()V"));
}

AndroidFramework::AndroidFramework()
: m_view(new ViewHandle(this)), m_work(m_view, 0)
{
  m_work.InitStorage(m_storage);
}

void AndroidFramework::SetParentView(JNIEnv * env, jobject view)
{
  m_env = env;
  m_parentView = view;
}

void AndroidFramework::InitRenderer()
{
  LOG(LDEBUG, ("AF::InitRenderer 1"));
  shared_ptr<yg::gl::RenderContext> pRC = CreateRenderContext();

  LOG(LDEBUG, ("AF::InitRenderer 2"));
  shared_ptr<yg::ResourceManager> pRM = CreateResourceManager();

  LOG(LDEBUG, ("AF::InitRenderer 3"));
  m_view->setRenderContext(pRC);
  m_view->setDrawer(CreateDrawer(pRM));

  LOG(LDEBUG, ("AF::InitRenderer 4"));
  m_work.initializeGL(pRC, pRM);

  //m_work.ShowAll();

  LOG(LDEBUG, ("AF::InitRenderer 5"));
}

void AndroidFramework::Resize(int w, int h)
{
  LOG(LDEBUG, ("AF::Resize 1"));
  m_view->drawer()->onSize(w, h);
  LOG(LDEBUG, ("AF::Resize 2"));
}

void AndroidFramework::DrawFrame()
{
//  m_work.Paint(make_shared_ptr(new PaintEvent(m_view->drawer())));
  yg::gl::Screen * screen = m_view->drawer()->screen().get();
  screen->beginFrame();
  screen->clear();
  yg::Color c(255, 0, 0, 255);

  m2::PointD centerPt(screen->width() / 2,
                      screen->height() / 2);

  m2::RectD r(centerPt.x - 100,
              centerPt.y - 50,
              centerPt.x + 100,
              centerPt.y + 50);

  screen->drawRectangle(r, c, yg::maxDepth);
  //screen->drawText(yg::FontDesc::defaultFont, centerPt, yg::EPosCenter, "Simplicity is the ultimate sophistication", yg::maxDepth, false);
  screen->endFrame();
}
