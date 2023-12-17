#import "UIColor+PartnerColor.h"

NS_ASSUME_NONNULL_BEGIN

@interface UIColor (MapsMeColor)

+ (UIColor *)blackPrimaryText;
+ (UIColor *)blackSecondaryText;
+ (UIColor *)blackHintText;
+ (UIColor *)red;
+ (UIColor *)white;
+ (UIColor *)primary;
+ (UIColor *)pressBackground;
+ (UIColor *)linkBlue;
+ (UIColor *)linkBlueHighlighted;
+ (UIColor *)buttonRed;
+ (UIColor *)blackDividers;
+ (UIColor *)whitePrimaryText;
+ (UIColor *)whitePrimaryTextHighlighted;
+ (UIColor *)whiteHintText;
+ (UIColor *)buttonDisabledBlueText;
+ (UIColor *)blackOpaque;
+ (UIColor *)bookingBackground;
+ (UIColor *)opentableBackground;
+ (UIColor *)transparentGreen;
+ (UIColor *)speedLimitRed;
+ (UIColor *)speedLimitGreen;
+ (UIColor *)speedLimitWhite;
+ (UIColor *)speedLimitLightGray;
+ (UIColor *)speedLimitDarkGray;

+ (UIColor *)colorWithName:(NSString *)colorName;
+ (UIColor *)colorFromHexString:(NSString *)hexString;

+ (void)setNightMode:(BOOL)mode;
+ (BOOL)isNightMode;

- (UIColor *)opposite;

@end

NS_ASSUME_NONNULL_END
