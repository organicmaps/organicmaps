/*
 * Framework.cpp
 *
 *  Created on: Oct 13, 2011
 *      Author: siarheirachytski
 */

#include "Framework.hpp"
#include "VideoTimer.hpp"

#include "../core/jni_helper.hpp"
#include "../core/render_context.hpp"

#include "../../../../../indexer/drawing_rules.hpp"

#include "../../../../../map/partial_render_policy.hpp"
#include "../../../../../map/framework.hpp"

#include "../../../../../std/shared_ptr.hpp"
#include "../../../../../std/bind.hpp"


#include "../../../../../yg/framebuffer.hpp"
#include "../../../../../yg/internal/opengl.hpp"

#include "../../../../../platform/platform.hpp"
#include "../../../../../base/logging.hpp"
#include "../../../../../base/math.hpp"

android::Framework * g_framework = 0;

namespace android
{
  void Framework::CallRepaint()
  {
    // Always get current env pointer, it's different for each thread
    JNIEnv * env;
    m_jvm->AttachCurrentThread(&env, NULL);
    env->CallVoidMethod(m_mainGLView, jni::GetJavaMethodID(env, m_mainGLView, "requestRender", "()V"));
  }

  Framework::Framework(JavaVM * jvm)
  : m_jvm(jvm),
    m_work()
  {
    ASSERT(g_framework == 0, ());
    g_framework = this;

    m_videoTimer = new VideoTimer(jvm, bind(&Framework::CallRepaint, this));

   // @TODO refactor storage
    m_work.Storage().ReInitCountries(false);
  }

  Framework::~Framework()
  {
    delete m_videoTimer;
  }

  void Framework::SetParentView(jobject view)
  {
    m_mainGLView = view;
  }

  namespace
  {
    struct make_all_invalid
    {
      size_t m_threadCount;

      make_all_invalid(size_t threadCount)
        : m_threadCount(threadCount)
      {}

      void operator() (int, int, int, drule::BaseRule * p)
      {
        for (size_t threadID = 0; threadID < m_threadCount; ++threadID)
        {
          p->MakeEmptyID(threadID);
          p->MakeEmptyID2(threadID);
        }
      }
    };
  }

  void Framework::InitRenderPolicy()
  {
    LOG(LDEBUG, ("AF::InitRenderer 1"));

    drule::rules().ForEachRule(make_all_invalid(GetPlatform().CpuCores() + 1));

    DrawerYG::Params params;
    params.m_frameBuffer = make_shared_ptr(new yg::gl::FrameBuffer(true));

    m_work.SetRenderPolicy(new PartialRenderPolicy(m_videoTimer, params, make_shared_ptr(new android::RenderContext())));

    LOG(LDEBUG, ("AF::InitRenderer 2"));

    m_work.ShowAll();

    LOG(LDEBUG, ("AF::InitRenderer 3"));
  }

  storage::Storage & Framework::Storage()
  {
    return m_work.Storage();
  }

  void Framework::Resize(int w, int h)
  {
    m_work.OnSize(w, h);
  }

  void Framework::DrawFrame()
  {
    if (m_work.NeedRedraw())
    {
      m_work.SetNeedRedraw(false);

      shared_ptr<PaintEvent> paintEvent(new PaintEvent(m_work.GetRenderPolicy()->GetDrawer().get()));

      m_work.BeginPaint(paintEvent);
      m_work.DoPaint(paintEvent);

      m_work.EndPaint(paintEvent);
    }
  }

  void Framework::Move(int mode, double x, double y)
  {
    DragEvent e(x, y);
    switch (mode)
    {
    case 0: m_work.StartDrag(e); break;
    case 1: m_work.DoDrag(e); break;
    case 2: m_work.StopDrag(e); break;
    }
  }

  void Framework::Zoom(int mode, double x1, double y1, double x2, double y2)
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

  void Framework::EnableLocation(bool enable)
  {
//    if (enable)
//      m_work.StartLocationService(bind(&f));
//    else
//      m_work.StopLocationService();
  }

  void Framework::UpdateLocation(uint64_t timestamp, double lat, double lon, float accuracy)
  {
    location::GpsInfo info;
    info.m_timestamp = static_cast<double>(timestamp);
    info.m_latitude = lat;
    info.m_longitude = lon;
    info.m_horizontalAccuracy = accuracy;
//    info.m_status = location::EAccurateMode;
//    info.m_altitude = 0;
//    info.m_course = 0;
//    info.m_verticalAccuracy = 0;
    m_work.OnGpsUpdate(info);
  }

  void Framework::UpdateCompass(uint64_t timestamp, double magneticNorth, double trueNorth, float accuracy)
  {
    location::CompassInfo info;
    info.m_timestamp = static_cast<double>(timestamp);
    info.m_magneticHeading = magneticNorth;
    info.m_trueHeading = trueNorth;
    info.m_accuracy = accuracy;
    m_work.OnCompassUpdate(info);
  }

  JavaVM * Framework::javaVM() const
  {
    return m_jvm;
  }
}
