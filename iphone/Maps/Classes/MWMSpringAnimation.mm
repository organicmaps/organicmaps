#import "MWMSpringAnimation.h"

@interface MWMSpringAnimation ()

@property (nonatomic) CGPoint velocity;
@property (nonatomic) CGPoint targetPoint;
@property (nonatomic) UIView * view;
@property (copy, nonatomic) TMWMVoidBlock completion;

@end

@implementation MWMSpringAnimation

+ (instancetype)animationWithView:(UIView *)view target:(CGPoint)target velocity:(CGPoint)velocity completion:(TMWMVoidBlock)completion
{
  return [[self alloc] initWithView:view target:target velocity:velocity completion:completion];
}

- (instancetype)initWithView:(UIView *)view target:(CGPoint)target velocity:(CGPoint)velocity completion:(TMWMVoidBlock)completion
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
  CGPoint const frictionForce = MultiplyCGPoint(self.velocity, frictionConstant);
  // spring force = (target point - current position) * spring constant
  CGPoint const springForce = MultiplyCGPoint(SubtractCGPoint(self.targetPoint, self.view.center), springConstant);
  // force = spring force - friction force
  CGPoint const force = SubtractCGPoint(springForce, frictionForce);
  // velocity = current velocity + force * time / mass
  self.velocity = AddCGPoint(self.velocity, MultiplyCGPoint(force, dt));
  // position = current position + velocity * time
  self.view.center = AddCGPoint(self.view.center, MultiplyCGPoint(self.velocity, dt));

  CGFloat const speed = LengthCGPoint(self.velocity);
  CGFloat const distanceToGoal = LengthCGPoint(SubtractCGPoint(self.targetPoint, self.view.center));
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
