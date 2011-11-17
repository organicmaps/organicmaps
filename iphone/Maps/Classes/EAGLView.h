#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>
#import <QuartzCore/CADisplayLink.h>
#import "MapViewController.h"

#include "../../std/shared_ptr.hpp"
#include "RenderBuffer.hpp"

class Framework;

namespace iphone
{
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
@public

  Framework * framework;
  VideoTimer * videoTimer;
	shared_ptr<iphone::RenderContext> renderContext;
	shared_ptr<yg::gl::FrameBuffer> frameBuffer;
	shared_ptr<iphone::RenderBuffer> renderBuffer;  
  RenderPolicy * renderPolicy;
}

- (void) initRenderPolicy;

@property (nonatomic, assign) Framework * framework;
@property (nonatomic, assign) VideoTimer * videoTimer;
@property (nonatomic, assign) shared_ptr<iphone::RenderContext> renderContext;
@property (nonatomic, assign) shared_ptr<iphone::RenderBuffer> renderBuffer;
@property (nonatomic, assign) shared_ptr<yg::gl::FrameBuffer> frameBuffer;
@property (nonatomic, assign) RenderPolicy * renderPolicy;

@end
