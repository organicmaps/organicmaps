//
//  EAGLView.h
//  OpenGLES_iPhone
//
//  Created by mmalc Crawford on 11/18/10.
//  Copyright 2010 Apple Inc. All rights reserved.
//

#import <UIKit/UIKit.h>

#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>

#include "../../std/shared_ptr.hpp"
#include "../../map/drawer_yg.hpp"
#include "RenderContext.hpp"
#include "RenderBuffer.hpp"

@class EAGLContext;

// This class wraps the CAEAGLLayer from CoreAnimation into a convenient UIView subclass.
// The view content is basically an EAGL surface you render your OpenGL scene into.
// Note that setting the view non-opaque will only work if the EAGL surface has an alpha channel.
@interface EAGLView : UIView {  
  
@public
  
	shared_ptr<yg::ResourceManager> resourceManager;
	shared_ptr<iphone::RenderContext> renderContext;
	shared_ptr<iphone::RenderBuffer> renderBuffer;
	shared_ptr<yg::gl::FrameBuffer> frameBuffer;
  shared_ptr<DrawerYG> drawer;
}

//@property (nonatomic, assign) shared_ptr<iphone::RenderContext> renderContext;
//@property (nonatomic, assign) shared_ptr<yg::ResourceManager> resourceManager;

@end
