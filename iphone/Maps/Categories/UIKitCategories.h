
#import <Foundation/Foundation.h>
#import "Macros.h"

static inline CGPoint CGPointSubtract(CGPoint p1, CGPoint p2)
{
  return CGPointMake(p1.x - p2.x, p1.y - p2.y);
}

static inline CGPoint CGPointAdd(CGPoint p1, CGPoint p2)
{
  return CGPointMake(p1.x + p2.x, p1.y + p2.y);
}

static inline CGPoint CGPointMultiply(CGPoint point, CGFloat multiplier)
{
  return CGPointMake(point.x * multiplier, point.y * multiplier);
}

static inline CGFloat CGPointLength(CGPoint point)
{
  return (CGFloat)sqrt(point.x * point.x + point.y * point.y);
}

@interface NSObject (Optimized)

+ (NSString *)className;
- (void)performAfterDelay:(NSTimeInterval)delayInSec block:(void (^)(void))block;

@end


@interface UIColor (HexColor)

+ (UIColor *)colorWithColorCode:(NSString *)colorCode;
+ (UIColor *)applicationBackgroundColor;
+ (UIColor *)applicationColor;
+ (UIColor *)navigationBarColor;

@end


@interface UIView (Coordinates)

@property (nonatomic) CGFloat minX;
@property (nonatomic) CGFloat minY;
@property (nonatomic) CGFloat midX;
@property (nonatomic) CGFloat midY;
@property (nonatomic) CGFloat maxX;
@property (nonatomic) CGFloat maxY;
@property (nonatomic) CGPoint origin;
@property (nonatomic) CGFloat width;
@property (nonatomic) CGFloat height;
@property (nonatomic) CGSize size;

+ (void)animateWithDuration:(NSTimeInterval)duration delay:(NSTimeInterval)delay damping:(double)dampingRatio initialVelocity:(double)springVelocity options:(UIViewAnimationOptions)options animations:(void (^)(void))animations completion:(void (^)(BOOL finished))completion;
- (void)sizeToIntegralFit;

@end


@interface UIApplication (URLs)

- (void)rateVersionFrom:(NSString *)launchPlaceName;

@end


@interface NSString (Size)

- (CGSize)sizeWithDrawSize:(CGSize)size font:(UIFont *)font;

@end


@interface SolidTouchView : UIView

@end


@interface SolidTouchImageView : UIImageView

@end


typedef void (^UIAlertViewBlock) (UIAlertView * alertView);
typedef void (^UIAlertViewCompletionBlock) (UIAlertView * alertView, NSInteger buttonIndex);

@interface UIAlertView (Blocks)

@property (copy, nonatomic) UIAlertViewCompletionBlock tapBlock;
@property (copy, nonatomic) UIAlertViewCompletionBlock willDismissBlock;
@property (copy, nonatomic) UIAlertViewCompletionBlock didDismissBlock;

@property (copy, nonatomic) UIAlertViewBlock willPresentBlock;
@property (copy, nonatomic) UIAlertViewBlock didPresentBlock;
@property (copy, nonatomic) UIAlertViewBlock cancelBlock;

@property (copy, nonatomic) BOOL(^shouldEnableFirstOtherButtonBlock)(UIAlertView * alertView);

@end

@interface UINavigationController (Autorotate)

- (BOOL)shouldAutorotate;
- (NSUInteger)supportedInterfaceOrientations;

@end