#import "WindowHandle.h"
#import "EAGLView.h"

namespace iphone
{
	WindowHandle::WindowHandle(bool & doRepaint)
  {
    m_doRepaint = &doRepaint;
	}

	void WindowHandle::invalidateImpl()
	{
    *m_doRepaint = true;
  }
}