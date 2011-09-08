#import "RenderContext.hpp"

#include "../../../map/window_handle.hpp"

@class EAGLView;

namespace iphone
{
  class WindowHandle : public ::WindowHandle
  {
    bool * m_doRepaint;

	public:
    WindowHandle(bool & doRepaint);
	  virtual void invalidateImpl();
  };
}
