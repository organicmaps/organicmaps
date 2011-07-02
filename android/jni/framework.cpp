#include "framework.h"
#include "jni_helper.h"
#include "rendering.h"

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
  /*
  LOG(LDEBUG, ("AF::DrawFrame 1"));
  shared_ptr<DrawerYG> p = m_view->drawer();

  LOG(LDEBUG, ("AF::DrawFrame 2"));
  p->beginFrame();

  LOG(LDEBUG, ("AF::DrawFrame 3"));
  p->clear();

  LOG(LDEBUG, ("AF::DrawFrame 4"));
  p->screen()->drawRectangle(m2::RectD(100, 100, 200, 200), yg::Color(255, 0, 0, 255), yg::maxDepth);

  LOG(LDEBUG, ("AF::DrawFrame 5"));
  p->endFrame();

  LOG(LDEBUG, ("AF::DrawFrame 6"));
  */
}
