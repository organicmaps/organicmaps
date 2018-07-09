#include "drape/pointers.hpp"
#include "drape/drape_global.hpp"

@class MWMMapWidgets;
namespace dp
{
  class ThreadSafeFactory;
}

// This class wraps the CAEAGLLayer from CoreAnimation into a convenient UIView subclass.
// The view content is basically an EAGL surface you render your OpenGL scene into.
// Note that setting the view non-opaque will only work if the EAGL surface has an alpha channel.
@interface EAGLView : UIView
{
  dp::ApiVersion m_apiVersion;
  drape_ptr<dp::ThreadSafeFactory> m_factory;
  // Do not call onSize from layoutSubViews when real size wasn't changed.
  // It's possible when we add/remove subviews (bookmark balloons) and it hangs the map without this check
  CGRect lastViewSize;
}

@property(nonatomic) MWMMapWidgets * widgetsManager;

@property(nonatomic, readonly) BOOL drapeEngineCreated;
@property(nonatomic) BOOL isLaunchByDeepLink;

@property(nonatomic, readonly) m2::PointU pixelSize;

- (void)deallocateNative;
- (CGPoint)viewPoint2GlobalPoint:(CGPoint)pt;
- (CGPoint)globalPoint2ViewPoint:(CGPoint)pt;
- (void)initialize;
- (void)setPresentAvailable:(BOOL)available;

@end
