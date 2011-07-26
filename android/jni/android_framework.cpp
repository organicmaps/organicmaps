#include "android_framework.hpp"
#include "jni_helper.h"
#include "rendering.h"

#include "../../base/logging.hpp"

#include "../../indexer/drawing_rules.hpp"

#include "../../map/render_policy_st.hpp"
#include "../../map/tiling_render_policy_st.hpp"
#include "../../map/framework.hpp"

#include "../../std/shared_ptr.hpp"
#include "../../std/bind.hpp"


void AndroidFramework::ViewHandle::invalidateImpl()
{
  m_parent->CallRepaint();
}

void AndroidFramework::CallRepaint()
{
  // Always get current env pointer, it's different for each thread
  JNIEnv * env;
  m_jvm->AttachCurrentThread(&env, NULL);
  env->CallVoidMethod(m_parentView, jni::GetJavaMethodID(env, m_parentView, "requestRender", "()V"));
}

AndroidFramework::AndroidFramework(JavaVM * jvm)
: m_jvm(jvm), m_view(new ViewHandle(this)), m_work(m_view, 0)
{
  m_work.InitStorage(m_storage);
  // @TODO refactor storage
  m_storage.ReInitCountries(false);
}

void AndroidFramework::SetParentView(jobject view)
{
  m_parentView = view;
}

namespace
{
  struct make_all_invalid
  {
    size_t m_threadCount;

    make_all_invalid(size_t threadCount)
      : m_threadCount(threadCount)
    {}

    void operator() (int, int, drule::BaseRule * p)
    {
      for (size_t threadID = 0; threadID < m_threadCount; ++threadID)
      {
        p->MakeEmptyID(threadID);
        p->MakeEmptyID2(threadID);
      }
    }
  };
}
void AndroidFramework::InitRenderer()
{
  LOG(LDEBUG, ("AF::InitRenderer 1"));

  drule::rules().ForEachRule(make_all_invalid(GetPlatform().CpuCores() + 1));

  // temporary workaround
  m_work.SetRenderPolicy(shared_ptr<RenderPolicy>(new RenderPolicyST(m_view, bind(&Framework<model::FeaturesFetcher>::DrawModel, &m_work, _1, _2, _3, _4))));

  m_view->setRenderContext(shared_ptr<yg::gl::RenderContext>());
  m_view->setDrawer(shared_ptr<DrawerYG>());

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

void f()
{
  // empty location stub
}

void AndroidFramework::EnableLocation(bool enable)
{
  if (enable)
    m_work.StartLocationService(bind(&f));
  else
    m_work.StopLocationService();
}

void AndroidFramework::UpdateLocation(uint64_t timestamp, double lat, double lon, float accuracy)
{
  location::GpsInfo info;
  info.m_timestamp = static_cast<double>(timestamp);
  info.m_latitude = lat;
  info.m_longitude = lon;
  info.m_horizontalAccuracy = accuracy;
  info.m_status = location::EAccurateMode;
  info.m_altitude = 0;
  info.m_course = 0;
  info.m_verticalAccuracy = 0;
  m_work.OnGpsUpdate(info);
}

void AndroidFramework::UpdateCompass(uint64_t timestamp, double magneticNorth, double trueNorth, float accuracy)
{
  location::CompassInfo info;
  info.m_timestamp = static_cast<double>(timestamp);
  info.m_magneticHeading = magneticNorth;
  info.m_trueHeading = trueNorth;
  info.m_accuracy = accuracy;
  m_work.OnCompassUpdate(info);
}
