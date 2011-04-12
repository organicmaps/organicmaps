//
//  EAGLView.m
//  OpenGLES_iPhone
//
//  Created by mmalc Crawford on 11/18/10.
//  Copyright 2010 Apple Inc. All rights reserved.
//

#import  <QuartzCore/QuartzCore.h>
#include "../../../yg/framebuffer.hpp"
#include "../../../yg/resource_manager.hpp"
#include "RenderContext.hpp"

#import "EAGLView.hpp"

@interface EAGLView (PrivateMethods)
- (void)createFramebuffer;
- (void)deleteFramebuffer;
@end

@implementation EAGLView

//@synthesize renderContext;
//@synthesize resourceManager;

// You must implement this method
+ (Class)layerClass
{
    return [CAEAGLLayer class];
}

//The EAGL view is stored in the nib file. When it's unarchived it's sent -initWithCoder:.
- (id)initWithCoder:(NSCoder*)coder
{
  self = [super initWithCoder:coder];
	if (self) 
  {
    CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
        
    eaglLayer.opaque = TRUE;
    eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                   [NSNumber numberWithBool:FALSE], kEAGLDrawablePropertyRetainedBacking,
                                   kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat,
                                   nil];
    
    renderContext.reset(new iphone::RenderContext());
    if (!renderContext)
    {
      [self release];
      return nil;
    }
    
    renderContext->makeCurrent();
    
    frameBuffer.reset(new yg::gl::FrameBuffer());
    
    
    int bigVBSize = pow(2, ceil(log2(15000 * sizeof(yg::gl::Vertex))));
    int bigIBSize = pow(2, ceil(log2(30000 * sizeof(unsigned short))));
    
    int smallVBSize = pow(2, ceil(log2(1500 * sizeof(yg::gl::Vertex))));
    int smallIBSize = pow(2, ceil(log2(3000 * sizeof(unsigned short))));
    
    int blitVBSize = pow(2, ceil(log2(10 * sizeof(yg::gl::AuxVertex))));
    int blitIBSize = pow(2, ceil(log2(10 * sizeof(unsigned short))));
    
    NSLog(@"Vendor: %s, Renderer: %s", glGetString(GL_VENDOR), glGetString(GL_RENDERER));
    
    yg::RtFormat fmt = yg::Rt4Bpp;
    
    resourceManager.reset(new yg::ResourceManager(
                           bigVBSize, bigIBSize, 4,
                           smallVBSize, smallIBSize, 10,
                           blitVBSize, blitIBSize, 10,
                           512, 256, 10,
                           GetPlatform().ReadPathForFile("unicode_blocks.txt").c_str(),
                           GetPlatform().ReadPathForFile("fonts_whitelist.txt").c_str(),
                           GetPlatform().ReadPathForFile("fonts_blacklist.txt").c_str(),
                           2000000,
                           fmt));
    resourceManager->addFonts(GetPlatform().GetFontNames());
    
    DrawerYG::params_t p;
    p.m_resourceManager = resourceManager;
    p.m_isMultiSampled = false;
    p.m_frameBuffer = frameBuffer;
    
    drawer.reset(new DrawerYG(GetPlatform().SkinName(), p));
  }
  
  self.multipleTouchEnabled = TRUE;
  
  return self;
}

- (void)dealloc
{
  [super dealloc];
}

- (void)layoutSubviews
{
  CGFloat scaleFactor = 2.0;
  if ([self respondsToSelector:@selector(contentScaleFactor)])
    self.contentScaleFactor = scaleFactor; 
  
  int width = self.frame.size.width * scaleFactor;
  int height = self.frame.size.height * scaleFactor;
  
  frameBuffer->resetRenderTarget();
  renderBuffer.reset();
  renderBuffer.reset(new iphone::RenderBuffer(renderContext, (CAEAGLLayer*)self.layer));
  frameBuffer->setRenderTarget(renderBuffer);
  frameBuffer->onSize(width, height);
  drawer->onSize(width, height);
}

@end
