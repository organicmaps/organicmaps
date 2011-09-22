#include "video_timer.hpp"

#include "../std/target_os.hpp"

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

  void start()
  {
    if (m_displayLink == 0)
    {
      CVDisplayLinkCreateWithActiveCGDisplays(&m_displayLink);
      CVDisplayLinkSetOutputCallback(m_displayLink, &displayLinkCallback, (void*)this);
      CVDisplayLinkStart(m_displayLink);
    }
  }

  static CVReturn displayLinkCallback(
     CVDisplayLinkRef displayLink,
     const CVTimeStamp *inNow,
     const CVTimeStamp *inOutputTime,
     CVOptionFlags flagsIn,
     CVOptionFlags *flagsOut,
     void *displayLinkContext
     )
  {
    AppleVideoTimer * t = reinterpret_cast<AppleVideoTimer*>(displayLinkContext);
    t->perform();

    return kCVReturnSuccess;
  }

  void stop()
  {
    if (m_displayLink)
    {
      CVDisplayLinkStop(m_displayLink);
      CVDisplayLinkRelease(m_displayLink);
      m_displayLink = 0;
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
