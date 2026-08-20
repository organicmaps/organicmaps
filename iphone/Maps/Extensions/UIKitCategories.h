#import <CoreApi/MWMTypes.h>

@interface NSObject (Optimized)

+ (NSString *)className;
- (void)performAfterDelay:(NSTimeInterval)delayInSec block:(MWMVoidBlock)block;

@end

@interface UIView (Coordinates)

@property(nonatomic) CGFloat minX;
@property(nonatomic) CGFloat minY;
@property(nonatomic) CGFloat midX;
@property(nonatomic) CGFloat midY;
@property(nonatomic) CGFloat maxX;
@property(nonatomic) CGFloat maxY;
@property(nonatomic) CGPoint origin;
@property(nonatomic) CGFloat width;
@property(nonatomic) CGFloat height;
@property(nonatomic) CGSize size;

- (void)sizeToIntegralFit;

@end

@interface UIView (Refresh)

@end

@interface UIApplication (URLs)

- (void)rateApp;

@end

@interface SolidTouchView : UIView

@end

@interface SolidTouchImageView : UIImageView

@end

@interface UIViewController (Safari)

/// Open URL internally in SFSafariViewController. Returns NO (false) if the url id invalid.
- (BOOL)openUrl:(NSString *)urlString;

/// Open URL externally in installed application (or in Safari if there are no appropriate application) if possible or
/// internally in SFSafariViewController. Returns NO (false) if the url id invalid.
///
/// @param urlString: URL string to open.
/// @param externally: If true, try to open URL in installed application or in Safari, otherwise open in internal
/// browser without leaving the app.
- (BOOL)openUrl:(NSString *)urlString externally:(BOOL)externally;

@end

@interface UIImage (ImageWithColor)

+ (UIImage *)imageWithColor:(UIColor *)color;

@end
