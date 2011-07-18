#include "framework.h"
#include "jni_helper.h"
#include "rendering.h"
#include "../../std/shared_ptr.hpp"

#include "../../base/logging.hpp"
#include "../../map/render_policy_st.hpp"
#include "../../std/shared_ptr.hpp"
#include "../../std/bind.hpp"
#include "../../map/framework.hpp"

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
  shared_ptr<RenderPolicy> renderPolicy(new RenderPolicyST(m_view, bind(&Framework<model::FeaturesFetcher>::DrawModel, &m_work, _1, _2, _3, _4)));
  m_work.SetRenderPolicy(renderPolicy);
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

  m_work.ShowAll();

  LOG(LDEBUG, ("AF::InitRenderer 5"));
}

void AndroidFramework::Resize(int w, int h)
{
  m_view->drawer()->onSize(w, h);
  m_work.OnSize(w, h);
}

void AndroidFramework::DrawFrame()
{
/*  yg::gl::Screen * screen = m_view->drawer()->screen().get();
  screen->beginFrame();
  screen->clear();

  m2::PointD centerPt(screen->width() / 2,
                      screen->height() / 2);

  m2::RectD r(centerPt.x - 100,
              centerPt.y - 50,
              centerPt.x + 100,
              centerPt.y + 50);

  screen->drawText(yg::FontDesc::defaultFont, centerPt, yg::EPosCenter, "Simplicity is the ultimate sophistication", yg::maxDepth, false);
  screen->drawRectangle(r, yg::Color(255, 0, 0, 255), yg::maxDepth);
  screen->drawRectangle(m2::Offset(r, m2::PointD(50, 50)), yg::Color(0, 255, 0, 255), yg::maxDepth);
  screen->endFrame();
*/
  m_work.Paint(make_shared_ptr(new PaintEvent(m_view->drawer())));
}
