#import <UIKit/UIKit.h>

@interface UIColor (MapsMeColor)

+ (UIColor *)primaryDark;
+ (UIColor *)primary;
+ (UIColor *)primaryLight;
+ (UIColor *)fadeBackground;
+ (UIColor *)pressBackground;
+ (UIColor *)red;
+ (UIColor *)orange;
+ (UIColor *)linkBlue;
+ (UIColor *)blackPrimaryText;
+ (UIColor *)blackSecondaryText;
+ (UIColor *)blackHintText;
+ (UIColor *)blackDividers;
+ (UIColor *)whitePrimaryText;
+ (UIColor *)whiteSecondaryText;
+ (UIColor *)whiteHintText;
+ (UIColor *)whiteDividers;
+ (UIColor *)buttonEnabledBlueText;
+ (UIColor *)buttonDisabledBlueText;
+ (UIColor *)buttonHighlightedBlueText;
+ (UIColor *)alertBackground;

+ (UIColor *)colorWithName:(NSString *)colorName;

@end
