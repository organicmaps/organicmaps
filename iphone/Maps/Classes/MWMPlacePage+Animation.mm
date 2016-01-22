#import "MWMPlacePage+Animation.h"
#import "MWMPlacePageViewManager.h"
#import <objc/runtime.h>

@implementation MWMPlacePage (Animation)

- (void)setSpringAnimation:(MWMSpringAnimation *)springAnimation
{
  objc_setAssociatedObject(self, @selector(springAnimation), springAnimation, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

- (MWMSpringAnimation *)springAnimation
{
  return objc_getAssociatedObject(self, @selector(springAnimation));
}

- (CGPoint)targetPoint
{
  [self doesNotRecognizeSelector:_cmd];
  return CGPointZero;
}

- (void)cancelSpringAnimation
{
  [self.manager.ownerViewController.view.animator removeAnimation:self.springAnimation];
  self.springAnimation = nil;
}

- (void)startAnimatingPlacePage:(MWMPlacePage *)placePage initialVelocity:(CGPoint)velocity completion:(TMWMVoidBlock)completion
{
  [self cancelSpringAnimation];
  self.springAnimation = [MWMSpringAnimation animationWithView:placePage.extendedPlacePageView target:placePage.targetPoint velocity:velocity completion:completion];
  [self.manager.ownerViewController.view.animator addAnimation:self.springAnimation];
}

@end
