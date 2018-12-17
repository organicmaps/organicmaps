#import "EAGLView.h"
#import "iosOGLContextFactory.h"
#import "MWMDirectionView.h"
#import "MWMMapWidgets.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "Framework.h"

#include "drape/drape_global.hpp"
#include "drape/visual_scale.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#ifdef OMIM_METAL_AVAILABLE
#import "MetalContextFactory.h"
#import <MetalKit/MetalKit.h>
#endif

@implementation EAGLView

namespace
{
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

+ (dp::ApiVersion)getSupportedApiVersion
{
  static dp::ApiVersion apiVersion = dp::ApiVersion::Invalid;
  if (apiVersion != dp::ApiVersion::Invalid)
    return apiVersion;
  
#ifdef OMIM_METAL_AVAILABLE
  if (@available(iOS 10.0, *))
  {
    if (GetFramework().LoadMetalAllowed())
    {
      id<MTLDevice> tempDevice = MTLCreateSystemDefaultDevice();
      if (tempDevice)
        apiVersion = dp::ApiVersion::Metal;
    }
  }
#endif
  
  if (apiVersion == dp::ApiVersion::Invalid)
  {
    EAGLContext * tempContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
    if (tempContext != nil)
      apiVersion = dp::ApiVersion::OpenGLES3;
    else
      apiVersion = dp::ApiVersion::OpenGLES2;
  }
  
  return apiVersion;
}

// You must implement this method
+ (Class)layerClass
{
#ifdef OMIM_METAL_AVAILABLE
  auto const apiVersion = [EAGLView getSupportedApiVersion];
  return apiVersion == dp::ApiVersion::Metal ? [CAMetalLayer class] : [CAEAGLLayer class];
#else
  return [CAEAGLLayer class];
#endif
}

// The GL view is stored in the nib file. When it's unarchived it's sent -initWithCoder:
- (id)initWithCoder:(NSCoder *)coder
{
  NSLog(@"EAGLView initWithCoder Started");
  self = [super initWithCoder:coder];
  if (self)
    [self initialize];

  NSLog(@"EAGLView initWithCoder Ended");
  return self;
}

- (void)initialize
{
  m_presentAvailable = false;
  m_lastViewSize = CGRectZero;
  m_apiVersion = [EAGLView getSupportedApiVersion];

  // Correct retina display support in renderbuffer.
  self.contentScaleFactor = [[UIScreen mainScreen] nativeScale];
  
  if (m_apiVersion == dp::ApiVersion::Metal)
  {
#ifdef OMIM_METAL_AVAILABLE
    CAMetalLayer * layer = (CAMetalLayer *)self.layer;
    layer.device = MTLCreateSystemDefaultDevice();
    NSAssert(layer.device != NULL, @"Metal is not supported on this device");
    layer.opaque = YES;
#endif
  }
  else
  {
    CAEAGLLayer * layer = (CAEAGLLayer *)self.layer;
    layer.opaque = YES;
    layer.drawableProperties = @{kEAGLDrawablePropertyRetainedBacking : @NO,
                                 kEAGLDrawablePropertyColorFormat : kEAGLColorFormatRGBA8};
  }
}

- (void)createDrapeEngine
{
  m2::PointU const s = [self pixelSize];
  
  if (m_apiVersion == dp::ApiVersion::Metal)
  {
#ifdef OMIM_METAL_AVAILABLE
    m_factory = make_unique_dp<MetalContextFactory>((CAMetalLayer *)self.layer, s);
#endif
  }
  else
  {
    m_factory = make_unique_dp<dp::ThreadSafeFactory>(
      new iosOGLContextFactory((CAEAGLLayer *)self.layer, m_apiVersion, m_presentAvailable));
  }
  [self createDrapeEngineWithWidth:s.x height:s.y];
}

- (void)createDrapeEngineWithWidth:(int)width height:(int)height
{
  LOG(LINFO, ("CreateDrapeEngine Started", width, height, m_apiVersion));
  CHECK(m_factory != nullptr, ());
  
  Framework::DrapeCreationParams p;
  p.m_apiVersion = m_apiVersion;
  p.m_surfaceWidth = width;
  p.m_surfaceHeight = height;
  p.m_visualScale = dp::VisualScale(getExactDPI(self.contentScaleFactor));
  p.m_hints.m_isFirstLaunch = [Alohalytics isFirstSession];
  p.m_hints.m_isLaunchByDeepLink = self.isLaunchByDeepLink;

  [self.widgetsManager setupWidgets:p];
  GetFramework().CreateDrapeEngine(make_ref(m_factory), move(p));

  self->_drapeEngineCreated = YES;
  LOG(LINFO, ("CreateDrapeEngine Finished"));
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

- (m2::PointU)pixelSize
{
  CGSize const s = self.bounds.size;
  uint32_t const w = static_cast<uint32_t>(s.width * self.contentScaleFactor);
  uint32_t const h = static_cast<uint32_t>(s.height * self.contentScaleFactor);
  return m2::PointU(w, h);
}

- (void)layoutSubviews
{
  if (!CGRectEqualToRect(m_lastViewSize, self.frame))
  {
    m_lastViewSize = self.frame;
    m2::PointU const s = [self pixelSize];
    GetFramework().OnSize(s.x, s.y);
    [self.widgetsManager resize:CGSizeMake(s.x, s.y)];
  }
  [super layoutSubviews];
}

- (void)deallocateNative
{
  GetFramework().PrepareToShutdown();
  m_factory.reset();
}

- (void)setPresentAvailable:(BOOL)available
{
  m_presentAvailable = available;
  if (m_factory != nullptr)
    m_factory->SetPresentAvailable(m_presentAvailable);
}

- (MWMMapWidgets *)widgetsManager
{
  if (!_widgetsManager)
    _widgetsManager = [[MWMMapWidgets alloc] init];
  return _widgetsManager;
}

@end
