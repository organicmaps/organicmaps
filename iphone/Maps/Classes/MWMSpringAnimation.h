#import "MWMAnimator.h"

@interface MWMSpringAnimation : NSObject <Animation>

@property (nonatomic, readonly) CGPoint velocity;

+ (instancetype)animationWithView:(UIView *)view target:(CGPoint)target velocity:(CGPoint)velocity completion:(TMWMVoidBlock)completion;

+ (CGFloat)approxTargetFor:(CGFloat)startValue velocity:(CGFloat)velocity;

@end
