#import "Common.h"
#import "EAGLView.h"
#import "MWMDirectionView.h"

#import "../Platform/opengl/iosOGLContextFactory.h"

#include "Framework.h"
#include "indexer/classificator_loader.hpp"

#include "platform/platform.hpp"

#include "std/bind.hpp"
#include "std/limits.hpp"
#include "std/unique_ptr.hpp"

@implementation EAGLView

// You must implement this method
+ (Class)layerClass
{
  return [CAEAGLLayer class];
}

// The GL view is stored in the nib file. When it's unarchived it's sent -initWithCoder:
- (id)initWithCoder:(NSCoder *)coder
{
  NSLog(@"EAGLView initWithCoder Started");

  if ((self = [super initWithCoder:coder]))
  {
    lastViewSize = CGRectZero;
    _widgetsManager = [[MWMMapWidgets alloc] init];

    // Setup Layer Properties
    CAEAGLLayer * eaglLayer = (CAEAGLLayer *)self.layer;

    eaglLayer.opaque = YES;
    eaglLayer.drawableProperties = @{kEAGLDrawablePropertyRetainedBacking : @NO,
                                     kEAGLDrawablePropertyColorFormat : kEAGLColorFormatRGBA8};
    
    // Correct retina display support in opengl renderbuffer
    self.contentScaleFactor = [self correctContentScale];

    m_factory = make_unique_dp<dp::ThreadSafeFactory>(new iosOGLContextFactory(eaglLayer));
  }

  NSLog(@"EAGLView initWithCoder Ended");
  return self;
}

- (void)createDrapeEngineWithWidth:(int)width height:(int)height
{
  NSLog(@"EAGLView createDrapeEngine Started");

  Framework::DrapeCreationParams p;
  p.m_surfaceWidth = width;
  p.m_surfaceHeight = height;
  p.m_visualScale = self.contentScaleFactor;
  p.m_initialMyPositionState = location::MODE_UNKNOWN_POSITION;

  [self.widgetsManager setupWidgets:p];
  GetFramework().CreateDrapeEngine(make_ref<dp::OGLContextFactory>(m_factory), move(p));

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

- (double)correctContentScale
{
  UIScreen * uiScreen = [UIScreen mainScreen];
  if (isIOSVersionLessThan(8))
    return uiScreen.scale;
  else
    return uiScreen.nativeScale;
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
