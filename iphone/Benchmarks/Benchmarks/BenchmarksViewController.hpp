//
//  BenchmarksViewController.h
//  Benchmarks
//
//  Created by Siarhei Rachytski on 4/11/11.
//  Copyright 2011 Credo-Dialogue. All rights reserved.
//

#import <UIKit/UIKit.h>

#import <OpenGLES/EAGL.h>

#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>

@interface BenchmarksViewController : UIViewController {
@private
    EAGLContext *context;
    GLuint program;
  
  int m_frameCount;
  double m_fullTime;
    
    BOOL animating;
    NSInteger animationFrameInterval;
    CADisplayLink *displayLink;
}

@property (readonly, nonatomic, getter=isAnimating) BOOL animating;
@property (nonatomic) NSInteger animationFrameInterval;

- (void)startAnimation;
- (void)stopAnimation;

@end
