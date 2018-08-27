#include "drape/pointers.hpp"
#include "drape/drape_global.hpp"

@class MetalView;
@class MWMMapWidgets;

namespace dp
{
  class GraphicsContextFactory;
}

// This class wraps the CAEAGLLayer from CoreAnimation into a convenient UIView subclass.
// The view content is basically an EAGL surface you render your OpenGL scene into.
// Note that setting the view non-opaque will only work if the EAGL surface has an alpha channel.
@interface EAGLView : UIView
{
  dp::ApiVersion m_apiVersion;
  drape_ptr<dp::GraphicsContextFactory> m_factory;
  // Do not call onSize from layoutSubViews when real size wasn't changed.
  // It's possible when we add/remove subviews (bookmark balloons) and it hangs the map without this check
  CGRect m_lastViewSize;
  bool m_presentAvailable;
}

@property(nonatomic) MWMMapWidgets * widgetsManager;
@property(weak, nonatomic) IBOutlet MetalView * metalView;

@property(nonatomic, readonly) BOOL drapeEngineCreated;
@property(nonatomic) BOOL isLaunchByDeepLink;
@property(nonatomic, readonly) m2::PointU pixelSize;

- (void)createDrapeEngine;
- (void)deallocateNative;
- (void)setPresentAvailable:(BOOL)available;
- (CGPoint)viewPoint2GlobalPoint:(CGPoint)pt;
- (CGPoint)globalPoint2ViewPoint:(CGPoint)pt;

@end
