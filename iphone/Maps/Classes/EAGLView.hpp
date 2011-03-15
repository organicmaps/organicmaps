//
//  EAGLView.h
//  chuchu_iphone
//
//  Created by Yury Melnichek on 08.03.09.
//  Copyright __MyCompanyName__ 2009. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#import "MapViewController.hpp"
#import "../../std/shared_ptr.hpp"
#import "../../map/drawer_yg.hpp"
#import "RenderBuffer.hpp"

namespace iphone
{
	class WindowHandle;
	class RenderContext;
	class RenderBuffer;
}

namespace yg
{
	namespace gl
	{
		class FrameBuffer;
	}
}

// This class wraps the CAEAGLLayer from CoreAnimation into a convenient UIView subclass.
// The view content is basically an EAGL surface you render your OpenGL scene into.
// Note that setting the view non-opaque will only work if the EAGL surface has an alpha channel.
@interface EAGLView : UIView
{
@private
	// The pixel dimensions of the backbuffer

	shared_ptr<iphone::RenderContext> renderContext;
	shared_ptr<iphone::RenderBuffer> renderBuffer;
	shared_ptr<yg::gl::FrameBuffer> frameBuffer;
  shared_ptr<DrawerYG> drawer;

@public

	shared_ptr<iphone::WindowHandle> windowHandle;
	shared_ptr<yg::ResourceManager> textureManager;

  MapViewController * controller;
}

/// Called as a result of invalidate on iphone::WindowHandle
- (void)drawView;
- (void)drawViewThunk:(id)obj;
- (void)drawViewOnMainThread;
- (void)onSize:(int)width withHeight:(int)height;

@property (nonatomic, assign) MapViewController * controller;
@property (nonatomic, assign) shared_ptr<iphone::WindowHandle> windowHandle;
@property (nonatomic, assign) shared_ptr<iphone::RenderContext> renderContext;
@property (nonatomic, assign) shared_ptr<yg::ResourceManager> resourceManager;

@end
