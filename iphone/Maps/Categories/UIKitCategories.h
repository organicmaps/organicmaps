
#import <Foundation/Foundation.h>

#define IPAD (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
#define DISPLAY_IS_4_INCH ([UIScreen mainScreen].bounds.size.height == 568.f)

#define SYSTEM_VERSION_IS_LESS_THAN(v) ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] == NSOrderedAscending)

#define KEYBOARD_HEIGHT 216.f

#define L(str) NSLocalizedString(str, nil)

@interface NSObject (Optimized)

+ (NSString *)className;
- (void)performAfterDelay:(NSTimeInterval)delay block:(void (^)(void))block;

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

@end


@interface UIApplication (URLs)

- (void)openProVersionFrom:(NSString *)launchPlaceName;
- (void)openGuideWithName:(NSString *)guideName itunesURL:(NSString *)itunesURL;

@end


@interface NSString (Size)

- (CGSize)sizeWithDrawSize:(CGSize)size font:(UIFont *)font;

@end


@interface SolidTouchView : UIView

@end
