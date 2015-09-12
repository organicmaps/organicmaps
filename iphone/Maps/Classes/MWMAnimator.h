#import <Foundation/Foundation.h>

@protocol Animation <NSObject>

- (void)animationTick:(CFTimeInterval)dt finished:(BOOL *)finished;

@end

@interface MWMAnimator : NSObject

+ (instancetype)animatorWithScreen:(UIScreen *)screen;
- (void)addAnimation:(id<Animation>)animatable;
- (void)removeAnimation:(id<Animation>)animatable;

@end

@interface UIView (AnimatorAdditions)

- (MWMAnimator *)animator;

@end
