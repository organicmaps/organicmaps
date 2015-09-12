#import "MWMPlacePage.h"
#import "MWMSpringAnimation.h"

@class MWMSpringAnimation;

@interface MWMPlacePage (Animation)

@property (nonatomic) MWMSpringAnimation * springAnimation;

- (void)cancelSpringAnimation;
- (void)startAnimatingPlacePage:(MWMPlacePage *)placePage initialVelocity:(CGPoint)velocity completion:(MWMSpringAnimationCompletionBlock)completion;
- (CGPoint)targetPoint;

@end
