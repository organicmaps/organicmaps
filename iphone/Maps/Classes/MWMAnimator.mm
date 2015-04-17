//
//  MWMAnimator.m
//  Maps
//
//  Created by v.mikhaylenko on 20.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMAnimator.h"
#import <objc/runtime.h>

static int kScreenAnimationKey;

@interface MWMAnimator ()

@property (nonatomic) CADisplayLink *displayLink;
@property (nonatomic) NSMutableSet *animations;

@end

@implementation MWMAnimator

+ (instancetype)animatorWithScreen:(UIScreen *)screen
{
  if (!screen)
    screen = [UIScreen mainScreen];

  MWMAnimator *animator = objc_getAssociatedObject(screen, &kScreenAnimationKey);
  if (!animator)
  {
    animator = [[self alloc] initWithScreen:screen];
    objc_setAssociatedObject(screen, &kScreenAnimationKey, animator, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  }

  return animator;
}

- (instancetype)initWithScreen:(UIScreen *)screen
{
  self = [super init];
  if (self)
  {
    self.displayLink = [screen displayLinkWithTarget:self selector:@selector(animationTick:)];
    self.displayLink.paused = YES;
    [self.displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
    self.animations = [NSMutableSet set];
  }
  return self;
}

- (void)animationTick:(CADisplayLink *)displayLink
{
  CFTimeInterval dt = displayLink.duration;
  for (id<Animation> a in self.animations.copy)
  {
    BOOL finished = NO;
    [a animationTick:dt finished:&finished];

    if (finished)
      [self.animations removeObject:a];

  }

  if (self.animations.count == 0)
    self.displayLink.paused = YES;
}

- (void)addAnimation:(id<Animation>)animation
{
  [self.animations addObject:animation];

  if (self.animations.count == 1)
    self.displayLink.paused = NO;
}

- (void)removeAnimation:(id <Animation>)animatable
{
  if (animatable == nil)
    return;

  [self.animations removeObject:animatable];

  if (self.animations.count == 0)
    self.displayLink.paused = YES;
}

@end

@implementation UIView (AnimatorAdditions)

- (MWMAnimator *)animator
{
  return [MWMAnimator animatorWithScreen:self.window.screen];
}

@end
