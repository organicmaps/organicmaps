#import <UIKit/UIKit.h>


#include "std/shared_ptr.hpp"
#include "indexer/map_style.hpp"

#ifndef USE_DRAPE

class VideoTimer;
class RenderPolicy;

namespace iphone
{
	class RenderContext;
	class RenderBuffer;
}

namespace graphics
{
	namespace gl
	{
		class FrameBuffer;
	}
}

#else

#include "drape/pointers.hpp"
namespace dp
{
  class OGLContextFactory;
}

#endif

// This class wraps the CAEAGLLayer from CoreAnimation into a convenient UIView subclass.
// The view content is basically an EAGL surface you render your OpenGL scene into.
// Note that setting the view non-opaque will only work if the EAGL surface has an alpha channel.
@interface EAGLView : UIView
{
#ifndef USE_DRAPE
  VideoTimer * videoTimer;
  shared_ptr<iphone::RenderContext> renderContext;
  shared_ptr<graphics::gl::FrameBuffer> frameBuffer;
  shared_ptr<iphone::RenderBuffer> renderBuffer;
  RenderPolicy * renderPolicy;
#else
  dp::MasterPointer<dp::OGLContextFactory> m_factory;
#endif
  // Do not call onSize from layoutSubViews when real size wasn't changed.
  // It's possible when we add/remove subviews (bookmark balloons) and it hangs the map without this check
  CGRect lastViewSize;
}

- (void)initRenderPolicy;
- (void)setMapStyle:(MapStyle)mapStyle;
- (CGPoint)viewPoint2GlobalPoint:(CGPoint)pt;
- (CGPoint)globalPoint2ViewPoint:(CGPoint)pt;

@end
