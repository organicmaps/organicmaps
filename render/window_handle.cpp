#include "window_handle.hpp"
#include "render_policy.hpp"

WindowHandle::WindowHandle() :
  m_hasPendingUpdates(false),
  m_isUpdatesEnabled(true),
  m_needRedraw(true),
  m_videoTimer(nullptr),
  m_stallsCount(0)
{
}

void WindowHandle::setRenderPolicy(RenderPolicy * renderPolicy)
{
  m_renderPolicy = renderPolicy;
}

void WindowHandle::setVideoTimer(VideoTimer * videoTimer)
{
  m_videoTimer = videoTimer;
  if (m_videoTimer != nullptr)
  {
    m_frameFn = videoTimer->frameFn();
    m_videoTimer->setFrameFn(bind(&WindowHandle::checkedFrameFn, this));
  }
  m_stallsCount = 0;
}

void WindowHandle::checkedFrameFn()
{
  if (m_renderPolicy->NeedRedraw())
    m_stallsCount = 0;
  else
    ++m_stallsCount;

  if (m_stallsCount >= 60)
  {
    //LOG(LINFO, ("PausedDOWN"));
    if (m_videoTimer != nullptr)
      m_videoTimer->pause();
  }
  else
  {
    if (m_frameFn != nullptr)
      m_frameFn();
  }
}

WindowHandle::~WindowHandle()
{
  if (m_videoTimer != nullptr)
  {
    m_videoTimer->stop();
    m_videoTimer->setFrameFn(m_frameFn);
  }
}

bool WindowHandle::needRedraw() const
{
  return m_isUpdatesEnabled && m_needRedraw;
}

void WindowHandle::checkTimer()
{
  if (m_videoTimer == nullptr)
    return;

  switch (m_videoTimer->state())
  {
  case VideoTimer::EStopped:
    m_videoTimer->start();
    break;
  case VideoTimer::EPaused:
    //LOG(LINFO, ("WokenUP"));
    m_videoTimer->resume();
    break;
  default:
    break;
  }
}

void WindowHandle::setNeedRedraw(bool flag)
{
  m_needRedraw = flag;
  if (m_needRedraw && m_isUpdatesEnabled)
    checkTimer();
}

shared_ptr<graphics::RenderContext> const & WindowHandle::renderContext()
{
  return m_renderContext;
}

void WindowHandle::setRenderContext(shared_ptr<graphics::RenderContext> const & renderContext)
{
  m_renderContext = renderContext;
}

bool WindowHandle::setUpdatesEnabled(bool doEnable)
{
  bool res = false;

  bool wasUpdatesEnabled = m_isUpdatesEnabled;
  m_isUpdatesEnabled = doEnable;

  if ((!wasUpdatesEnabled) && (doEnable) && (m_hasPendingUpdates))
  {
    setNeedRedraw(true);
    m_hasPendingUpdates = false;
    res = true;
  }

  return res;
}

void WindowHandle::invalidate()
{
  if (m_isUpdatesEnabled)
    setNeedRedraw(true);
  else
    m_hasPendingUpdates = true;
}
