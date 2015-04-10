#include "platform/video_timer.hpp"

#include "std/target_os.hpp"

#include <CoreVideo/CVDisplayLink.h>

class AppleVideoTimer : public VideoTimer
{

  CVDisplayLinkRef m_displayLink;

public:

  AppleVideoTimer(VideoTimer::TFrameFn frameFn)
    : VideoTimer(frameFn), m_displayLink(0)
  {}

  ~AppleVideoTimer()
  {
    stop();
  }

  static CVReturn displayLinkCallback(
     CVDisplayLinkRef /*displayLink*/,
     const CVTimeStamp * /*inNow*/,
     const CVTimeStamp * /*inOutputTime*/,
     CVOptionFlags /*flagsIn*/,
     CVOptionFlags * /*flagsOut*/,
     void * displayLinkContext
     )
  {
    AppleVideoTimer * t = reinterpret_cast<AppleVideoTimer*>(displayLinkContext);
    t->perform();

    return kCVReturnSuccess;
  }

  void start()
  {
    if (m_displayLink == 0)
    {
      CVDisplayLinkCreateWithActiveCGDisplays(&m_displayLink);
      CVDisplayLinkSetOutputCallback(m_displayLink, &displayLinkCallback, (void*)this);
      resume();
    }
  }

  void resume()
  {
    CVDisplayLinkStart(m_displayLink);
    m_state = ERunning;
  }

  void pause()
  {
    CVDisplayLinkStop(m_displayLink);
    m_state = EPaused;
  }

  void stop()
  {
    if (m_displayLink)
    {
      if (state() == ERunning)
        pause();
      CVDisplayLinkRelease(m_displayLink);
      m_displayLink = 0;
      m_state = EStopped;
    }
  }

  void perform()
  {
    m_frameFn();
  }
};

VideoTimer * CreateAppleVideoTimer(VideoTimer::TFrameFn frameFn)
{
  return new AppleVideoTimer(frameFn);
}
