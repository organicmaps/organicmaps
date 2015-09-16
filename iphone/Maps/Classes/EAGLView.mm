#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>
#import "Common.h"
#import "EAGLView.h"

#import "../Categories/UIKitCategories.h"

#include "Framework.h"

#ifndef USE_DRAPE
  #include "RenderBuffer.hpp"
  #include "RenderContext.hpp"
  #include "graphics/resource_manager.hpp"
  #include "graphics/opengl/opengl.hpp"
  #include "graphics/data_formats.hpp"
  #include "indexer/classificator_loader.hpp"
#else
  #import "../Platform/opengl/iosOGLContextFactory.h"
#endif

#include "render/render_policy.hpp"

#include "platform/platform.hpp"
#include "platform/video_timer.hpp"

#include "std/bind.hpp"


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
    // Setup Layer Properties
    CAEAGLLayer * eaglLayer = (CAEAGLLayer *)self.layer;

    eaglLayer.opaque = YES;
    // ColorFormat : RGB565
    // Backbuffer : YES, (to prevent from loosing content when mixing with ordinary layers).
    eaglLayer.drawableProperties = @{kEAGLDrawablePropertyRetainedBacking : @NO, kEAGLDrawablePropertyColorFormat : kEAGLColorFormatRGB565};
    
    // Correct retina display support in opengl renderbuffer
    self.contentScaleFactor = [self correctContentScale];

#ifndef USE_DRAPE
    renderContext = shared_ptr<iphone::RenderContext>(new iphone::RenderContext());

    if (!renderContext.get())
    {
      NSLog(@"EAGLView initWithCoder Error");
      return nil;
    }
    
    renderContext->makeCurrent();

    typedef void (*drawFrameFn)(id, SEL);
    SEL drawFrameSel = @selector(drawFrame);
    drawFrameFn drawFrameImpl = (drawFrameFn)[self methodForSelector:drawFrameSel];
    
    videoTimer = CreateIOSVideoTimer(bind(drawFrameImpl, self, drawFrameSel));
#else
    dp::ThreadSafeFactory * factory = new dp::ThreadSafeFactory(new iosOGLContextFactory(eaglLayer));
    m_factory.Reset(factory);
#endif
  }

  NSLog(@"EAGLView initWithCoder Ended");
  return self;
}

- (void)initRenderPolicy
{
  NSLog(@"EAGLView initRenderPolicy Started");
  
#ifndef USE_DRAPE
  graphics::ResourceManager::Params rmParams;
  rmParams.m_videoMemoryLimit = GetPlatform().VideoMemoryLimit();
  rmParams.m_texFormat = graphics::Data4Bpp;

  RenderPolicy::Params rpParams;

  UIScreen * screen = [UIScreen mainScreen];
  CGRect screenRect = screen.bounds;

  double vs = self.contentScaleFactor;

  rpParams.m_screenWidth = screenRect.size.width * vs;
  rpParams.m_screenHeight = screenRect.size.height * vs;

  rpParams.m_skinName = "basic.skn";

  if (vs == 1.0)
    rpParams.m_density = graphics::EDensityMDPI;
  else if (vs > 2.0)
    rpParams.m_density = graphics::EDensityIPhone6Plus;
  else
    rpParams.m_density = graphics::EDensityXHDPI;

  rpParams.m_videoTimer = videoTimer;
  rpParams.m_useDefaultFB = false;
  rpParams.m_rmParams = rmParams;
  rpParams.m_primaryRC = renderContext;

  try
  {
    renderPolicy = CreateRenderPolicy(rpParams);
  }
  catch (graphics::gl::platform_unsupported const & )
  {
    /// terminate program (though this situation is unreal :) )
  }

  frameBuffer = renderPolicy->GetDrawer()->Screen()->frameBuffer();

  Framework & f = GetFramework();
  f.SetRenderPolicy(renderPolicy);
  f.InitGuiSubsystem();
#else
  CGRect frameRect = [UIScreen mainScreen].applicationFrame;
  GetFramework().CreateDrapeEngine(m_factory.GetRefPointer(), self.contentScaleFactor, frameRect.size.width, frameRect.size.height);
#endif

  NSLog(@"EAGLView initRenderPolicy Ended");
}

- (void)setMapStyle:(MapStyle)mapStyle
{
  Framework & f = GetFramework();
  
  if (f.GetMapStyle() == mapStyle)
    return;
  
  NSLog(@"EAGLView setMapStyle Started");
  
  renderContext->makeCurrent();
  
  /// drop old render policy
  f.SetRenderPolicy(nullptr);
  frameBuffer.reset();

  f.SetMapStyle(mapStyle);
  
  /// init new render policy
  [self initRenderPolicy];
  
  /// restore render policy screen
  CGFloat const scale = self.contentScaleFactor;
  CGSize const s = self.bounds.size;
  [self onSize:s.width * scale withHeight:s.height * scale];
  
  /// update framework
  f.SetUpdatesEnabled(true);
  
  NSLog(@"EAGLView setMapStyle Ended");
}

- (void)onSize:(int)width withHeight:(int)height
{
#ifndef USE_DRAPE
  frameBuffer->onSize(width, height);
  
  graphics::Screen * screen = renderPolicy->GetDrawer()->Screen();

  /// free old render buffer, as we would not create a new one.
  screen->resetRenderTarget();
  screen->resetDepthBuffer();
  renderBuffer.reset();
  
  /// detaching of old render target will occur inside beginFrame
  screen->beginFrame();
  screen->endFrame();

	/// allocate the new one
  renderBuffer.reset();
  renderBuffer.reset(new iphone::RenderBuffer(renderContext, (CAEAGLLayer*)self.layer));
  
  screen->setRenderTarget(renderBuffer);
  screen->setDepthBuffer(make_shared<graphics::gl::RenderBuffer>(width, height, true));
#endif

  GetFramework().OnSize(width, height);
  
#ifndef USE_DRAPE
  screen->beginFrame();
  screen->clear(graphics::Screen::s_bgColor);
  screen->endFrame();
  GetFramework().SetNeedRedraw(true);
#endif
}

- (double)correctContentScale
{
  UIScreen * uiScreen = [UIScreen mainScreen];
  if (isIOSVersionLessThan(8))
    return uiScreen.scale;
  else
    return uiScreen.nativeScale;
}

#ifndef USE_DRAPE
- (void)drawFrame
{
	shared_ptr<PaintEvent> pe(new PaintEvent(renderPolicy->GetDrawer().get()));
  
  Framework & f = GetFramework();
  if (f.NeedRedraw())
  {
    f.SetNeedRedraw(false);
    f.BeginPaint(pe);
    f.DoPaint(pe);
    renderBuffer->present();
    f.EndPaint(pe);
  }
}
#endif

- (void)layoutSubviews
{
  if (!CGRectEqualToRect(lastViewSize, self.frame))
  {
    lastViewSize = self.frame;
#ifndef USE_DRAPE
    CGFloat const scale = self.contentScaleFactor;
    CGSize const s = self.bounds.size;
	  [self onSize:s.width * scale withHeight:s.height * scale];
#else
    CGSize const s = self.bounds.size;
    [self onSize:s.width withHeight:s.height];
#endif
  }
}

- (void)dealloc
{
#ifndef USE_DRAPE
  delete videoTimer;
  [EAGLContext setCurrentContext:nil];
#else
  GetFramework().PrepareToShutdown();
  m_factory.Destroy();
#endif
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
