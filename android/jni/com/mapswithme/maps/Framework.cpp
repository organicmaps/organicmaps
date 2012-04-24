#include "Framework.hpp"
#include "VideoTimer.hpp"

#include "../core/jni_helper.hpp"
#include "../core/render_context.hpp"

#include "../../../../../indexer/drawing_rules.hpp"

#include "../../../../../map/framework.hpp"

#include "../../../../../yg/framebuffer.hpp"
#include "../../../../../yg/internal/opengl.hpp"

#include "../../../../../platform/platform.hpp"
#include "../../../../../platform/location.hpp"

#include "../../../../../base/logging.hpp"
#include "../../../../../base/math.hpp"

#include "../../../../../std/shared_ptr.hpp"
#include "../../../../../std/bind.hpp"


namespace android
{
  void Framework::CallRepaint()
  {
    //LOG(LINFO, ("Calling Repaint"));
  }

  Framework::Framework()
   : m_work(),
     m_eventType(NVMultiTouchEventType(0)),
     m_hasFirst(false),
     m_hasSecond(false),
     m_mask(0),
     m_isInsideDoubleClick(false),
     m_isCleanSingleClick(false)
  {
    ASSERT(g_framework == 0, ());
    g_framework = this;

    m_videoTimer = new VideoTimer(bind(&Framework::CallRepaint, this));

    size_t const measurementsCount = 5;
    m_sensors[0].SetCount(measurementsCount);
    m_sensors[1].SetCount(measurementsCount);

   // @TODO refactor storage
    m_work.Storage().ReInitCountries(false);
  }

  void Framework::SetEmptyModelMessage(string const & emptyModelMsg)
  {
    m_work.GetInformationDisplay().setEmptyModelMessage(emptyModelMsg.c_str());
  }

  Framework::~Framework()
  {
    delete m_videoTimer;
  }

  void Framework::OnLocationStatusChanged(int newStatus)
  {
    m_work.OnLocationStatusChanged(static_cast<location::TLocationStatus>(newStatus));
  }

  void Framework::OnLocationUpdated(uint64_t time, double lat, double lon, float accuracy)
  {
    location::GpsInfo info;
    info.m_timestamp = static_cast<double>(time);
    info.m_latitude = lat;
    info.m_longitude = lon;
    info.m_horizontalAccuracy = accuracy;
    m_work.OnGpsUpdate(info);
  }

  void Framework::OnCompassUpdated(uint64_t timestamp, double magneticNorth, double trueNorth, float accuracy)
  {
    location::CompassInfo info;
    info.m_timestamp = static_cast<double>(timestamp);
    info.m_magneticHeading = magneticNorth;
    info.m_trueHeading = trueNorth;
    info.m_accuracy = accuracy;
    m_work.OnCompassUpdate(info);
  }

  void Framework::UpdateCompassSensor(int ind, float * arr)
  {
    //LOG ( LINFO, ("Sensors before, C++: ", arr[0], arr[1], arr[2]) );
    m_sensors[ind].Next(arr);
    //LOG ( LINFO, ("Sensors after, C++: ", arr[0], arr[1], arr[2]) );
  }

  void Framework::DeleteRenderPolicy()
  {
    m_work.SaveState();
    LOG(LINFO, ("clearing current render policy."));
    m_work.SetRenderPolicy(0);
  }

  bool Framework::InitRenderPolicy()
  {
    yg::ResourceManager::Params rmParams;
    rmParams.m_videoMemoryLimit = 30 * 1024 * 1024;
    rmParams.m_rtFormat = yg::Data8Bpp;
    rmParams.m_texFormat = yg::Data4Bpp;

    try
    {
      m_work.SetRenderPolicy(CreateRenderPolicy(m_videoTimer,
                                                true,
                                                rmParams,
                                                make_shared_ptr(new android::RenderContext())));
      LoadState();
    }
    catch (yg::gl::platform_unsupported const & e)
    {
      LOG(LINFO, ("this android platform is unsupported, reason=", e.what()));
      return false;
    }

    m_work.SetUpdatesEnabled(true);

    return true;
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

      NVEventSwapBuffersEGL();

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

  void Framework::Touch(int action, int mask, double x1, double y1, double x2, double y2)
  {
    NVMultiTouchEventType eventType = (NVMultiTouchEventType)action;

    /// processing double-click
    if ((mask != 0x1) || (eventType == NV_MULTITOUCH_CANCEL))
    {
      //cancelling double click
      m_isInsideDoubleClick = false;
      m_isCleanSingleClick = false;
    }
    else
    {
      if (eventType == NV_MULTITOUCH_DOWN)
      {
        m_isCleanSingleClick = true;
        m_lastX1 = x1;
        m_lastY1 = y1;
      }

      if (eventType == NV_MULTITOUCH_MOVE)
      {
        if ((fabs(x1 - m_lastX1) > 10)
        ||  (fabs(y1 - m_lastY1) > 10))
          m_isCleanSingleClick = false;
      }

      if ((eventType == NV_MULTITOUCH_UP) && (m_isCleanSingleClick))
      {
        if (m_isInsideDoubleClick)
        {
          if (m_doubleClickTimer.ElapsedSeconds() <= 0.5)
          {
            // performing double-click
            m_isInsideDoubleClick = false;
            m_work.ScaleToPoint(ScaleToPointEvent(x1, y1, 1.5));
          }
          else
          {
            // restarting double click
            m_isInsideDoubleClick = true;
            m_doubleClickTimer.Reset();
          }
        }
        else
        {
          // starting double click
          m_isInsideDoubleClick = true;
          m_doubleClickTimer.Reset();
        }
      }
    }

    /// general case processing

    if (m_mask != mask)
    {
      if (m_mask == 0x0)
      {
        if (mask == 0x1)
          m_work.StartDrag(DragEvent(x1, y1));

        if (mask == 0x2)
          m_work.StartDrag(DragEvent(x2, y2));

        if (mask == 0x3)
          m_work.StartScale(ScaleEvent(x1, y1, x2, y2));
      }

      if (m_mask == 0x1)
      {
        m_work.StopDrag(DragEvent(x1, y1));

        if (mask == 0x0)
        {
          if ((eventType != NV_MULTITOUCH_UP) && (eventType != NV_MULTITOUCH_CANCEL))
            LOG(LINFO, ("should be NV_MULTITOUCH_UP or NV_MULTITOUCH_CANCEL"));
        }

        if (m_mask == 0x2)
          m_work.StartDrag(DragEvent(x2, y2));

        if (mask == 0x3)
          m_work.StartScale(ScaleEvent(x1, y1, x2, y2));
      }

      if (m_mask == 0x2)
      {
        m_work.StopDrag(DragEvent(x2, y2));

        if (mask == 0x0)
        {
          if ((eventType != NV_MULTITOUCH_UP) && (eventType != NV_MULTITOUCH_CANCEL))
            LOG(LINFO, ("should be NV_MULTITOUCH_UP or NV_MULTITOUCH_CANCEL"));
        }

        if (mask == 0x1)
          m_work.StartDrag(DragEvent(x1, y1));

        if (mask == 0x3)
          m_work.StartScale(ScaleEvent(x1, y1, x2, y2));
      }

      if (m_mask == 0x3)
      {
        m_work.StopScale(ScaleEvent(m_x1, m_y1, m_x2, m_y2));

        if ((eventType == NV_MULTITOUCH_MOVE))
        {
          if (mask == 0x1)
            m_work.StartDrag(DragEvent(x1, y1));

          if (mask == 0x2)
            m_work.StartDrag(DragEvent(x2, y2));
        }
        else
          mask = 0;
      }
    }
    else
    {
      if (eventType == NV_MULTITOUCH_MOVE)
      {
        if (m_mask == 0x1)
          m_work.DoDrag(DragEvent(x1, y1));
        if (m_mask == 0x2)
          m_work.DoDrag(DragEvent(x2, y2));
        if (m_mask == 0x3)
          m_work.DoScale(ScaleEvent(x1, y1, x2, y2));
      }

      if ((eventType == NV_MULTITOUCH_CANCEL) || (eventType == NV_MULTITOUCH_UP))
      {
        if (m_mask == 0x1)
          m_work.StopDrag(DragEvent(x1, y1));
        if (m_mask == 0x2)
          m_work.StopDrag(DragEvent(x2, y2));
        if (m_mask == 0x3)
          m_work.StopScale(ScaleEvent(m_x1, m_y1, m_x2, m_y2));
        mask = 0;
      }
    }

    m_x1 = x1;
    m_y1 = y1;
    m_x2 = x2;
    m_y2 = y2;
    m_mask = mask;
    m_eventType = eventType;

  }

  void Framework::LoadState()
  {
    if (!m_work.LoadState())
    {
      LOG(LINFO, ("no saved state, showing all world"));
      m_work.ShowAll();
    }
    else
    {
      LOG(LINFO, ("state loaded successfully"));
    }
  }

  void Framework::SaveState()
  {
    m_work.SaveState();
  }

  void Framework::Invalidate()
  {
    m_work.Invalidate();
  }

  void Framework::SetupMeasurementSystem()
  {
    m_work.SetupMeasurementSystem();
  }
}
