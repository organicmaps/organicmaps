#import "RenderContext.hpp"

#include "../../../map/window_handle.hpp"

@class EAGLView;

namespace iphone
{
  class WindowHandle : public ::WindowHandle
  {
    EAGLView * m_view;

	public:
    WindowHandle(EAGLView * view);
	  virtual void invalidateImpl();
  };
}
