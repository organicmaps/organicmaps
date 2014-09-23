#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>

#import "../Categories/UIKitCategories.h"

#import "EAGLView.h"

#include "RenderBuffer.hpp"
#include "RenderContext.hpp"
#include "Framework.h"

#include "../../graphics/resource_manager.hpp"
#include "../../graphics/opengl/opengl.hpp"
#include "../../graphics/data_formats.hpp"

#include "../../map/render_policy.hpp"

#include "../../platform/platform.hpp"
#include "../../platform/video_timer.hpp"

#include "../../std/bind.hpp"


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

    renderContext = shared_ptr<iphone::RenderContext>(new iphone::RenderContext());

    if (!renderContext.get())
    {
      NSLog(@"EAGLView initWithCoder Error");
      return nil;
    }
    
    renderContext->makeCurrent();
    
    // ColorFormat : RGB565
    // Backbuffer : YES, (to prevent from loosing content when mixing with ordinary layers).
    eaglLayer.drawableProperties = @{kEAGLDrawablePropertyRetainedBacking : @NO, kEAGLDrawablePropertyColorFormat : kEAGLColorFormatRGB565};
    // Correct retina display support in opengl renderbuffer
    self.contentScaleFactor = [self correctContentScale];
  }

  NSLog(@"EAGLView initWithCoder Ended");
  return self;
}

- (void)initRenderPolicy
{
  NSLog(@"EAGLView initRenderPolicy Started");
  
  typedef void (*drawFrameFn)(id, SEL);
  SEL drawFrameSel = @selector(drawFrame);
  drawFrameFn drawFrameImpl = (drawFrameFn)[self methodForSelector:drawFrameSel];

  videoTimer = CreateIOSVideoTimer(bind(drawFrameImpl, self, drawFrameSel));

  graphics::ResourceManager::Params rmParams;
  rmParams.m_videoMemoryLimit = GetPlatform().VideoMemoryLimit();
  rmParams.m_texFormat = graphics::Data4Bpp;

  RenderPolicy::Params rpParams;

  UIScreen * screen = [UIScreen mainScreen];
  CGRect frameRect = screen.applicationFrame;
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

  frameBuffer = renderPolicy->GetDrawer()->screen()->frameBuffer();

  Framework & f = GetFramework();
  f.OnSize(frameRect.size.width * vs, frameRect.size.height * vs);
  f.SetRenderPolicy(renderPolicy);
  f.InitGuiSubsystem();

  NSLog(@"EAGLView initRenderPolicy Ended");
}

- (void)onSize:(int)width withHeight:(int)height
{
  frameBuffer->onSize(width, height);
  
  graphics::Screen * screen = renderPolicy->GetDrawer()->screen();

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

  GetFramework().OnSize(width, height);
  
  screen->beginFrame();
  screen->clear(graphics::Screen::s_bgColor);
  screen->endFrame();
}

- (double)correctContentScale
{
  UIScreen * uiScreen = [UIScreen mainScreen];
  if (SYSTEM_VERSION_IS_LESS_THAN(@"8"))
    return uiScreen.scale;
  else
    return uiScreen.nativeScale;
}

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

- (void)layoutSubviews
{
  if (!CGRectEqualToRect(lastViewSize, self.frame))
  {
    lastViewSize = self.frame;
    CGFloat const scale = self.contentScaleFactor;
    CGSize const s = self.bounds.size;
	  [self onSize:s.width * scale withHeight:s.height * scale];
  }
}

- (void)dealloc
{
  delete videoTimer;
  [EAGLContext setCurrentContext:nil];
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
