/*
 *  WindowHandle.mm
 *  Maps
 *
 *  Created by Siarhei Rachytski on 8/15/10.
 *  Copyright 2010 OMIM. All rights reserved.
 *
 */


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