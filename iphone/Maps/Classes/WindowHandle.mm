#import "WindowHandle.h"
#import "EAGLView.h"

namespace iphone
{
	WindowHandle::WindowHandle(EAGLView * view)
	{
		m_view = view;
	}

	void WindowHandle::invalidateImpl()
	{
		[m_view drawViewOnMainThread];
	}
}