//
//  MWMSpringAnimation.m
//  Maps
//
//  Created by v.mikhaylenko on 21.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMSpringAnimation.h"
#import "UIKitCategories.h"

@interface MWMSpringAnimation ()

@property (nonatomic) CGPoint velocity;
@property (nonatomic) CGPoint targetPoint;
@property (nonatomic) UIView * view;
@property (copy, nonatomic) MWMSpringAnimationCompletionBlock completion;

@end

@implementation MWMSpringAnimation

+ (instancetype)animationWithView:(UIView *)view target:(CGPoint)target velocity:(CGPoint)velocity completion:(MWMSpringAnimationCompletionBlock)completion
{
  return [[self alloc] initWithView:view target:target velocity:velocity completion:completion];
}

- (instancetype)initWithView:(UIView *)view target:(CGPoint)target velocity:(CGPoint)velocity completion:(MWMSpringAnimationCompletionBlock)completion
{
  self = [super init];
  if (self)
  {
    self.view = view;
    self.targetPoint = target;
    self.velocity = velocity;
    self.completion = completion;
  }
  return self;
}

- (void)animationTick:(CFTimeInterval)dt finished:(BOOL *)finished
{
  CGFloat const frictionConstant = 25.;
  CGFloat const springConstant = 300.;

  // friction force = velocity * friction constant
  CGPoint const frictionForce = CGPointMultiply(self.velocity, frictionConstant);
  // spring force = (target point - current position) * spring constant
  CGPoint const springForce = CGPointMultiply(CGPointSubtract(self.targetPoint, self.view.center), springConstant);
  // force = spring force - friction force
  CGPoint const force = CGPointSubtract(springForce, frictionForce);
  // velocity = current velocity + force * time / mass
  self.velocity = CGPointAdd(self.velocity, CGPointMultiply(force, dt));
  // position = current position + velocity * time
  self.view.center = CGPointAdd(self.view.center, CGPointMultiply(self.velocity, dt));

  CGFloat const speed = CGPointLength(self.velocity);
  CGFloat const distanceToGoal = CGPointLength(CGPointSubtract(self.targetPoint, self.view.center));
  if (speed < 0.05 && distanceToGoal < 1)
  {
    self.view.center = self.targetPoint;
    *finished = YES;
    if (self.completion)
      self.completion();
  }
}

+ (CGFloat)approxTargetFor:(CGFloat)startValue velocity:(CGFloat)velocity
{
  CGFloat const decelaration = (velocity > 0 ? -1.0 : 1.0) * 300.0;
  return startValue - (velocity * velocity) / (2.0 * decelaration);
}

@end
