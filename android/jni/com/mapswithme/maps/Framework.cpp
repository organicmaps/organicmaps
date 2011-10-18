/*
 * Framework.cpp
 *
 *  Created on: Oct 13, 2011
 *      Author: siarheirachytski
 */

#include "Framework.hpp"

#include "../core/jni_helper.hpp"
#include "../core/render_context.hpp"

#include "../../../../../indexer/drawing_rules.hpp"

#include "../../../../../map/render_policy_st.hpp"
#include "../../../../../map/tiling_render_policy_st.hpp"
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
    m_handle(new WindowHandle()),
    m_work(m_handle, 0)
  {
    ASSERT(g_framework == 0, ());
    g_framework = this;

    m_work.InitStorage(m_storage);
    // @TODO refactor storage
    m_storage.ReInitCountries(false);
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

  void Framework::CreateDrawer()
  {
    Platform & pl = GetPlatform();

    DrawerYG::params_t p;
    p.m_resourceManager = m_rm;
    p.m_glyphCacheID = m_rm->guiThreadGlyphCacheID();
    p.m_frameBuffer = make_shared_ptr(new yg::gl::FrameBuffer(true));
    p.m_skinName = pl.SkinName();

    m_drawer = make_shared_ptr(new DrawerYG(p));
  }

  void Framework::CreateResourceManager()
  {
    int bigVBSize = pow(2, ceil(log(1500.0 * sizeof(yg::gl::Vertex)) / log(2)));
    int bigIBSize = pow(2, ceil(log(3000.0 * sizeof(unsigned short)) / log(2)));

    int smallVBSize = pow(2, ceil(log(1500.0 * sizeof(yg::gl::Vertex)) / log(2)));
    int smallIBSize = pow(2, ceil(log(3000.0 * sizeof(unsigned short)) / log(2)));

    int blitVBSize = pow(2, ceil(log(10.0 * sizeof(yg::gl::AuxVertex)) / log(2)));
    int blitIBSize = pow(2, ceil(log(10.0 * sizeof(unsigned short)) / log(2)));

    Platform & pl = GetPlatform();
    m_rm = make_shared_ptr(new yg::ResourceManager(
          bigVBSize, bigIBSize, 40,
          smallVBSize, smallIBSize, 10,
          blitVBSize, blitIBSize, 10,
          512, 256, 6,
          512, 256, 4,
          "unicode_blocks.txt",
          "fonts_whitelist.txt",
          "fonts_blacklist.txt",
          2 * 1024 * 1024,
          1,
          yg::Rt8Bpp,
          false));

    Platform::FilesList fonts;
    pl.GetFontNames(fonts);
    m_rm->addFonts(fonts);
  }

  void Framework::InitRenderer()
  {
    LOG(LDEBUG, ("AF::InitRenderer 1"));

    drule::rules().ForEachRule(make_all_invalid(GetPlatform().CpuCores() + 1));

    // temporary workaround
    m_work.SetRenderPolicy(shared_ptr<RenderPolicy>(new RenderPolicyST(m_handle, bind(&::Framework<model::FeaturesFetcher>::DrawModel, &m_work, _1, _2, _3, _4, _5, false))));

    m_rc = make_shared_ptr(new android::RenderContext());

    LOG(LDEBUG, ("AF::InitRenderer 2"));
    CreateResourceManager();

    LOG(LDEBUG, ("AF::InitRenderer 3"));
    //m_view->setRenderContext(pRC);
    CreateDrawer();

    LOG(LDEBUG, ("AF::InitRenderer 4"));
    m_work.InitializeGL(m_rc, m_rm);

    LOG(LDEBUG, ("AF::InitRenderer 5"));

    m_work.ShowAll();

    LOG(LDEBUG, ("AF::InitRenderer 6"));
  }

  storage::Storage & Framework::Storage()
  {
    return m_storage;
  }

  void Framework::Resize(int w, int h)
  {
    m_drawer->onSize(w, h);
    m_work.OnSize(w, h);
  }

  void Framework::DrawFrame()
  {
    if (m_work.NeedRedraw())
    {
      m_work.SetNeedRedraw(false);

      shared_ptr<PaintEvent> paintEvent(new PaintEvent(m_drawer.get()));

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
    if (enable)
      m_work.StartLocationService(bind(&f));
    else
      m_work.StopLocationService();
  }

  void Framework::UpdateLocation(uint64_t timestamp, double lat, double lon, float accuracy)
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
