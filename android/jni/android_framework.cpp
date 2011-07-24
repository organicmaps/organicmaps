#include "android_framework.hpp"
#include "jni_helper.h"
#include "rendering.h"
#include "../../std/shared_ptr.hpp"

#include "../../base/logging.hpp"
#include "../../map/render_policy_st.hpp"
#include "../../map/tiling_render_policy_st.hpp"
#include "../../std/shared_ptr.hpp"
#include "../../std/bind.hpp"
#include "../../map/framework.hpp"

void AndroidFramework::ViewHandle::invalidateImpl()
{
  m_parent->CallRepaint();
}

void AndroidFramework::CallRepaint()
{
  // Always get current env pointer, it's different for each thread
  JNIEnv * env;
  m_jvm->AttachCurrentThread(&env, NULL);
  LOG(LDEBUG, ("AF::CallRepaint", env));
  env->CallVoidMethod(m_parentView, jni::GetJavaMethodID(env, m_parentView, "requestRender", "()V"));
}

AndroidFramework::AndroidFramework(JavaVM * jvm)
: m_jvm(jvm), m_view(new ViewHandle(this)), m_work(m_view, 0)
{
  m_work.InitStorage(m_storage);
  shared_ptr<RenderPolicy> renderPolicy(new RenderPolicyST(m_view, bind(&Framework<model::FeaturesFetcher>::DrawModel, &m_work, _1, _2, _3, _4)));
  m_work.SetRenderPolicy(renderPolicy);
  // @TODO refactor storage
  m_storage.ReInitCountries(false);
}

void AndroidFramework::SetParentView(jobject view)
{
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

  m_work.ShowAll();

  LOG(LDEBUG, ("AF::InitRenderer 6"));
}

void AndroidFramework::Resize(int w, int h)
{
  m_view->drawer()->onSize(w, h);
  m_work.OnSize(w, h);
}

void AndroidFramework::DrawFrame()
{
  m_work.Paint(make_shared_ptr(new PaintEvent(m_view->drawer())));
}

void AndroidFramework::Move(int mode, double x, double y)
{
  DragEvent e(x, y);
  switch (mode)
  {
  case 0: m_work.StartDrag(e); break;
  case 1: m_work.DoDrag(e); break;
  case 2: m_work.StopDrag(e); break;
  }
}

void AndroidFramework::Zoom(int mode, double x1, double y1, double x2, double y2)
{
  ScaleEvent e(x1, y1, x2, y2);
  switch (mode)
  {
  case 0: m_work.StartScale(e); break;
  case 1: m_work.DoScale(e); break;
  case 2: m_work.StopScale(e); break;
  }
}
