#import "Common.h"
#import "EAGLView.h"
#import "MapsAppDelegate.h"
#import "LocationManager.h"
#import "MWMDirectionView.h"

#import "../Platform/opengl/iosOGLContextFactory.h"

#include "Framework.h"
#include "indexer/classificator_loader.hpp"

#include "platform/platform.hpp"

#include "drape/visual_scale.hpp"

#include "std/bind.hpp"
#include "std/limits.hpp"
#include "std/unique_ptr.hpp"

@implementation EAGLView

namespace
{
// Returns native scale if it's possible.
double correctContentScale()
{
  UIScreen * uiScreen = [UIScreen mainScreen];
  
  if (isIOSVersionLessThan(8))
    return [uiScreen respondsToSelector:@selector(scale)] ? [uiScreen scale] : 1.f;
  else
    return [uiScreen respondsToSelector:@selector(nativeScale)] ? [uiScreen nativeScale] : 1.f;
}

// Returns DPI as exact as possible. It works for iPhone, iPad and iWatch.
double getExactDPI(double contentScaleFactor)
{
  float const iPadDPI = 132.f;
  float const iPhoneDPI = 163.f;
  float const mDPI = 160.f;

  switch (UI_USER_INTERFACE_IDIOM())
  {
    case UIUserInterfaceIdiomPhone:
      return iPhoneDPI * contentScaleFactor;
    case UIUserInterfaceIdiomPad:
      return iPadDPI * contentScaleFactor;
    default:
      return mDPI * contentScaleFactor;
  }
}
} //  namespace

// You must implement this method
+ (Class)layerClass
{
  return [CAEAGLLayer class];
}

// The GL view is stored in the nib file. When it's unarchived it's sent -initWithCoder:
- (id)initWithCoder:(NSCoder *)coder
{
  BOOL const isDaemon = MapsAppDelegate.theApp.m_locationManager.isDaemonMode;
  NSLog(@"EAGLView initWithCoder Started");
  self = [super initWithCoder:coder];
  if (self && !isDaemon)
    [self initialize];

  NSLog(@"EAGLView initWithCoder Ended");
  return self;
}

- (void)initialize
{
  lastViewSize = CGRectZero;
  _widgetsManager = [[MWMMapWidgets alloc] init];

  // Setup Layer Properties
  CAEAGLLayer * eaglLayer = (CAEAGLLayer *)self.layer;

  eaglLayer.opaque = YES;
  eaglLayer.drawableProperties = @{kEAGLDrawablePropertyRetainedBacking : @NO,
                                   kEAGLDrawablePropertyColorFormat : kEAGLColorFormatRGBA8};

  // Correct retina display support in opengl renderbuffer
  self.contentScaleFactor = correctContentScale();

  m_factory = make_unique_dp<dp::ThreadSafeFactory>(new iosOGLContextFactory(eaglLayer));
}

- (void)createDrapeEngineWithWidth:(int)width height:(int)height
{
  NSLog(@"EAGLView createDrapeEngine Started");
  if (MapsAppDelegate.theApp.m_locationManager.isDaemonMode)
    return;

  Framework::DrapeCreationParams p;
  p.m_surfaceWidth = width;
  p.m_surfaceHeight = height;
  p.m_visualScale = dp::VisualScale(getExactDPI(self.contentScaleFactor));

  [self.widgetsManager setupWidgets:p];
  GetFramework().CreateDrapeEngine(make_ref<dp::OGLContextFactory>(m_factory), move(p));

  _drapeEngineCreated = YES;

  NSLog(@"EAGLView createDrapeEngine Ended");
}

- (void)addSubview:(UIView *)view
{
  [super addSubview:view];
  for (UIView * v in self.subviews)
  {
    if ([v isKindOfClass:[MWMDirectionView class]])
    {
      [self bringSubviewToFront:v];
      break;
    }
  }
}

- (void)applyOnSize:(int)width withHeight:(int)height
{
  dispatch_async(dispatch_get_main_queue(), ^
  {
    GetFramework().OnSize(width, height);
    [self.widgetsManager resize:CGSizeMake(width, height)];
  });
}

- (void)onSize:(int)width withHeight:(int)height
{
  int w = width * self.contentScaleFactor;
  int h = height * self.contentScaleFactor;

  if (GetFramework().GetDrapeEngine() == nullptr)
  {
    [self createDrapeEngineWithWidth:w height:h];
    GetFramework().LoadState();
    return;
  }

  [self applyOnSize:w withHeight:h];
}

- (void)layoutSubviews
{
  if (!CGRectEqualToRect(lastViewSize, self.frame))
  {
    lastViewSize = self.frame;
    CGSize const s = self.bounds.size;
    [self onSize:s.width withHeight:s.height];
  }
}

- (void)deallocateNative
{
  GetFramework().PrepareToShutdown();
  m_factory.reset();
}

- (CGPoint)viewPoint2GlobalPoint:(CGPoint)pt
{
  CGFloat const scaleFactor = self.contentScaleFactor;
  m2::PointD const ptG = GetFramework().PtoG(m2::PointD(pt.x * scaleFactor, pt.y * scaleFactor));
  return CGPointMake(ptG.x, ptG.y);
}

- (CGPoint)globalPoint2ViewPoint:(CGPoint)pt
{
  CGFloat const scaleFactor = self.contentScaleFactor;
  m2::PointD const ptP = GetFramework().GtoP(m2::PointD(pt.x, pt.y));
  return CGPointMake(ptP.x / scaleFactor, ptP.y / scaleFactor);
}

@end
