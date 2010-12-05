/*
 *  WindowHandle.hpp
 *  Maps
 *
 *  Created by Siarhei Rachytski on 8/15/10.
 *  Copyright 2010 OMIM. All rights reserved.
 *
 */

#include "../../../map/window_handle.hpp"
#include "../../../yg/screen.hpp"
#include "RenderContext.hpp"
#import "EAGLView.hpp"

namespace iphone
{ 
  class WindowHandle : public ::WindowHandle
  {
	private:
  	EAGLView * m_view;
	public:
  	WindowHandle(EAGLView * view);
	
	  void invalidate();
  };
}
