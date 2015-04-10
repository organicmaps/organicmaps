#include "platform/video_timer.hpp"

#include "std/target_os.hpp"

#import <QuartzCore/CADisplayLink.h>
#import <Foundation/NSRunLoop.h>

class IOSVideoTimer;

@interface VideoTimerWrapper : NSObject {
@private
  IOSVideoTimer * m_timer;
}
- (id)initWithTimer:(IOSVideoTimer *) timer;
- (void)perform;
@end

class IOSVideoTimer : public VideoTimer
{

  VideoTimerWrapper * m_objCppWrapper;
  CADisplayLink * m_displayLink;

public:

  IOSVideoTimer(VideoTimer::TFrameFn frameFn) : VideoTimer(frameFn), m_objCppWrapper(0), m_displayLink(0)
  {}

  ~IOSVideoTimer()
  {
    stop();
  }

  void start()
  {
    if (m_displayLink == 0)
    {
      m_objCppWrapper = [[VideoTimerWrapper alloc] initWithTimer:this];
      m_displayLink = [CADisplayLink displayLinkWithTarget:m_objCppWrapper selector:@selector(perform)];
      m_displayLink.frameInterval = 1;
      m_displayLink.paused = true;
      [m_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
      resume();
    }
  }

  void stop()
  {
    if (m_displayLink)
    {
      // Set EStopped flag first. It seems like 'CADisplayLink::invalidate' can invoke pending 'perform'.
      // So we should check EStopped flag in 'perform' to skip pending call.
      m_state = EStopped;
      [m_displayLink invalidate];
      [m_objCppWrapper release];
      m_displayLink = 0;
    }
  }

  void pause()
  {
    m_displayLink.paused = true;
    m_state = EPaused;
  }

  void resume()
  {
    m_displayLink.paused = false;
    m_state = ERunning;
  }

  void perform()
  {
    // In case when we stopped and have pending perform at the same time.
    // It's not allowed to call m_frameFn after stopping (see WindowHandle::~WindowHandle).
    if (m_state != EStopped)
      m_frameFn();
  }
};

@implementation VideoTimerWrapper

- (id)initWithTimer:(IOSVideoTimer*) timer
{
  self = [super init];
  if (self) {
    m_timer = timer;
  }
  return self;
}

- (void)dealloc
{
  [super dealloc];
}

- (void)perform
{
  m_timer->perform();
}

@end

VideoTimer * CreateIOSVideoTimer(VideoTimer::TFrameFn frameFn)
{
  return new IOSVideoTimer(frameFn);
}


