#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>

#import "EAGLView.hpp"

#include "../../yg/screen.hpp"
#include "../../yg/texture.hpp"
#include "../../yg/resource_manager.hpp"
#include "../../yg/internal/opengl.hpp"
#include "../../yg/skin.hpp"
#include "IPhonePlatform.hpp"
#include "WindowHandle.hpp"
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
		
		/// ColorFormat : RGBA8
		/// Backbuffer : NO
    eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                    [NSNumber numberWithBool:NO],
                                    kEAGLDrawablePropertyRetainedBacking,
                                    kEAGLColorFormatRGBA8,
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

		resourceManager = shared_ptr<yg::ResourceManager>(new yg::ResourceManager(
						15000 * sizeof(yg::gl::Vertex), 30000 * sizeof(unsigned short), 20,
						1500 * sizeof(yg::gl::Vertex), 3000 * sizeof(unsigned short), 100,
					  512, 256, 10,
					  2000000));

//		resourceManager->addFont(GetPlatform().ReadPathForFile("dejavusans.ttf").c_str());
		resourceManager->addFont(GetPlatform().ReadPathForFile("wqy-microhei.ttf").c_str());		
		
		drawer = shared_ptr<DrawerYG>(new DrawerYG(resourceManager, GetPlatform().SkinName(), !GetPlatform().IsMultiSampled()));
		drawer->setFrameBuffer(frameBuffer);
		
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
	[[self controller] onResize:self.frame.size.width * self.contentScaleFactor withHeight:self.frame.size.height * self.contentScaleFactor];
	[self onSize:self.frame.size.width * self.contentScaleFactor withHeight:self.frame.size.height * self.contentScaleFactor];
	[self drawView];
}

- (void)dealloc
{
  [EAGLContext setCurrentContext:nil];
  [super dealloc];
}


@end
