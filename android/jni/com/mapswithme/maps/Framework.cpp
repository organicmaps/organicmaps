#include "Framework.hpp"
#include "VideoTimer.hpp"

#include "../core/jni_helper.hpp"
#include "../core/render_context.hpp"

#include "../../../../../indexer/drawing_rules.hpp"

#include "../../../../../map/framework.hpp"

#include "../../../../../std/shared_ptr.hpp"
#include "../../../../../std/bind.hpp"


#include "../../../../../yg/framebuffer.hpp"
#include "../../../../../yg/internal/opengl.hpp"

#include "../../../../../platform/platform.hpp"
#include "../../../../../platform/location.hpp"

#include "../../../../../base/logging.hpp"
#include "../../../../../base/math.hpp"

extern android::Framework * g_framework;

namespace android
{

Framework & GetFramework()
{
  ASSERT(g_framework, ("Framework is not initialized"));
  return *g_framework;
}

Framework::Framework()
{
 // @TODO refactor storage
  m_work.Storage().ReInitCountries(false);
}

Framework::~Framework()
{
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

void Framework::DeleteRenderPolicy()
{
  m_work.SaveState();
  LOG(LDEBUG, ("clearing current render policy."));
  m_work.SetRenderPolicy(0);
}

bool Framework::InitRenderPolicy()
{
  LOG(LDEBUG, ("AF::InitRenderer 1"));

  yg::ResourceManager::Params rmParams;
  rmParams.m_videoMemoryLimit = 10 * 1024 * 1024;
  rmParams.m_rtFormat = yg::Data4Bpp;
  rmParams.m_texFormat = yg::Data4Bpp;

  try
  {
    m_work.SetRenderPolicy(CreateRenderPolicy(&m_videoTimer,
                                              true,
                                              rmParams,
                                              make_shared_ptr(new android::RenderContext())));
    m_work.LoadState();
  }
  catch (yg::gl::platform_unsupported const & e)
  {
    LOG(LINFO, ("this android platform is unsupported, reason=", e.what()));
    return false;
  }

  m_work.SetUpdatesEnabled(true);

  LOG(LDEBUG, ("AF::InitRenderer 3"));

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
    m_work.EndPaint(paintEvent);
  }
}

void Framework::Move(int mode, double x, double y)
{
  DragEvent const e(x, y);
  switch (mode)
  {
  case 0: m_work.StartDrag(e); break;
  case 1: m_work.DoDrag(e); break;
  case 2: m_work.StopDrag(e); break;
  }
}

void Framework::Zoom(int mode, double x1, double y1, double x2, double y2)
{
  ScaleEvent const e(x1, y1, x2, y2);
  switch (mode)
  {
  case 0: m_work.StartScale(e); break;
  case 1: m_work.DoScale(e); break;
  case 2: m_work.StopScale(e); break;
  }
}

void Framework::LoadState()
{
  if (!m_work.LoadState())
  {
    LOG(LDEBUG, ("no saved state, showing all world"));
    m_work.ShowAll();
  }
  else
    LOG(LDEBUG, ("state loaded successfully"));
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

} // namespace android
