#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>

#import "EAGLView.h"
#import "WindowHandle.h"

#include "../../yg/screen.hpp"
#include "../../yg/texture.hpp"
#include "../../yg/resource_manager.hpp"
#include "../../yg/internal/opengl.hpp"
#include "../../yg/skin.hpp"
#include "IPhonePlatform.hpp"
#include "RenderBuffer.hpp"
#include "RenderContext.hpp"

// A class extension to declare private methods
@interface EAGLView ()

@end

@implementation EAGLView

@synthesize controller;
@synthesize windowHandle;
@synthesize renderContext;
@synthesize resourceManager;

// You must implement this method
+ (Class)layerClass
{
  return [CAEAGLLayer class];
}

// The GL view is stored in the nib file. When it's unarchived it's sent -initWithCoder:
- (id)initWithCoder:(NSCoder*)coder
{
  if ((self = [super initWithCoder:coder]))
  {
      // Setup Layer Properties
      CAEAGLLayer * eaglLayer = (CAEAGLLayer *)self.layer;

      eaglLayer.opaque = YES;

      /// ColorFormat : RGB565
      /// Backbuffer : YES, (to prevent from loosing content when mixing with ordinary layers).
      eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                      [NSNumber numberWithBool:NO],
                                      kEAGLDrawablePropertyRetainedBacking,
                                      kEAGLColorFormatRGB565,
                                      kEAGLDrawablePropertyColorFormat,
                                      nil];

      int etalonW = 320;
      int scrW = etalonW;

      UIDevice * device = [UIDevice currentDevice];

      float ver = [device.systemVersion floatValue];
      NSLog(@"%@", device.systemVersion);
      /// rounding problems
      if (ver >= 3.199)
      {
        UIScreen * mainScr = [UIScreen mainScreen];
		scrW = mainScr.currentMode.size.width;
        if (scrW == 640)
            self.contentScaleFactor = 2.0;
      }

      renderContext = shared_ptr<iphone::RenderContext>(new iphone::RenderContext());

      if (!renderContext.get())
      {
          [self release];
          return nil;
      }

      renderContext->makeCurrent();
      frameBuffer = shared_ptr<yg::gl::FrameBuffer>(new yg::gl::FrameBuffer());

      int bigVBSize = pow(2, ceil(log2(15000 * sizeof(yg::gl::Vertex))));
      int bigIBSize = pow(2, ceil(log2(30000 * sizeof(unsigned short))));

      int smallVBSize = pow(2, ceil(log2(1500 * sizeof(yg::gl::Vertex))));
      int smallIBSize = pow(2, ceil(log2(3000 * sizeof(unsigned short))));

      int blitVBSize = pow(2, ceil(log2(10 * sizeof(yg::gl::AuxVertex))));
      int blitIBSize = pow(2, ceil(log2(10 * sizeof(unsigned short))));

      resourceManager = shared_ptr<yg::ResourceManager>(new yg::ResourceManager(
						bigVBSize, bigIBSize, 20,
						smallVBSize, smallIBSize, 30,
						blitVBSize, blitIBSize, 20,
						512, 256, 10,
						GetPlatform().ReadPathForFile("unicode_blocks.txt").c_str(),
						GetPlatform().ReadPathForFile("fonts_whitelist.txt").c_str(),
 						GetPlatform().ReadPathForFile("fonts_blacklist.txt").c_str(),
						2000000));


		resourceManager->addFonts(GetPlatform().GetFontNames());

		DrawerYG::params_t p;
		p.m_resourceManager = resourceManager;
		p.m_isMultiSampled = false;
		p.m_frameBuffer = frameBuffer;

		drawer = shared_ptr<DrawerYG>(new DrawerYG(GetPlatform().SkinName(), p));

//		frameBuffer->onSize(renderBuffer->width(), renderBuffer->height());
//		frameBuffer->setRenderTarget(renderBuffer);

		windowHandle = shared_ptr<iphone::WindowHandle>(new iphone::WindowHandle(self));
		windowHandle->setDrawer(drawer);
		windowHandle->setRenderContext(renderContext);
  }

  self.multipleTouchEnabled = YES;

  return self;
}

- (void)onSize:(int)width withHeight:(int)height
{
	/// free old video memory
	frameBuffer->resetRenderTarget();
	renderBuffer.reset();

	/// allocate the new one
	renderBuffer = shared_ptr<iphone::RenderBuffer>(new iphone::RenderBuffer(renderContext, (CAEAGLLayer*)self.layer));
	frameBuffer->setRenderTarget(renderBuffer);
	frameBuffer->onSize(width, height);
	drawer->onSize(width, height);
}

- (void)drawView
{
	[controller onPaint];
	renderBuffer->present();
}

- (void)drawViewThunk:(id)obj
{
	[self drawView];
}

- (void)drawViewOnMainThread
{
	[self performSelectorOnMainThread:@selector(drawViewThunk:) withObject:nil waitUntilDone:NO];
}

- (void)layoutSubviews
{
	NSLog(@"layoutSubviews");

  CGFloat scaleFactor = 1.0;
  if ([self respondsToSelector:@selector(contentScaleFactor)])
  	scaleFactor = self.contentScaleFactor;

	[[self controller] onResize:self.frame.size.width * scaleFactor withHeight:self.frame.size.height * scaleFactor];
	[self onSize:self.frame.size.width * scaleFactor withHeight:self.frame.size.height * scaleFactor];
	[self drawView];
}

- (void)dealloc
{
  [EAGLContext setCurrentContext:nil];
  [super dealloc];
}


@end
