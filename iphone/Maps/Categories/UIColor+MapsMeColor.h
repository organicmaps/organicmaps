#import "UIColor+PartnerColor.h"

NS_ASSUME_NONNULL_BEGIN

@interface UIColor (MapsMeColor)

+ (UIColor *)blackPrimaryText __attribute__ ((deprecated));
+ (UIColor *)blackSecondaryText __attribute__ ((deprecated));
+ (UIColor *)blackHintText __attribute__ ((deprecated));
+ (UIColor *)red __attribute__ ((deprecated));
+ (UIColor *)white __attribute__ ((deprecated));
+ (UIColor *)primary __attribute__ ((deprecated));
+ (UIColor *)pressBackground __attribute__ ((deprecated));
+ (UIColor *)linkBlue __attribute__ ((deprecated));
+ (UIColor *)linkBlueHighlighted __attribute__ ((deprecated));
+ (UIColor *)buttonRed __attribute__ ((deprecated));
+ (UIColor *)blackDividers __attribute__ ((deprecated));
+ (UIColor *)whitePrimaryText __attribute__ ((deprecated));
+ (UIColor *)whitePrimaryTextHighlighted __attribute__ ((deprecated));
+ (UIColor *)whiteHintText __attribute__ ((deprecated));
+ (UIColor *)buttonDisabledBlueText __attribute__ ((deprecated));
+ (UIColor *)blackOpaque __attribute__ ((deprecated));
+ (UIColor *)bookingBackground __attribute__ ((deprecated));
+ (UIColor *)opentableBackground __attribute__ ((deprecated));
+ (UIColor *)bannerBackground __attribute__ ((deprecated));
+ (UIColor *)transparentGreen __attribute__ ((deprecated));
+ (UIColor *)speedLimitRed __attribute__ ((deprecated));
+ (UIColor *)speedLimitGeen __attribute__ ((deprecated));
+ (UIColor *)speedLimitWhite __attribute__ ((deprecated));
+ (UIColor *)speedLimitLightGray __attribute__ ((deprecated));
+ (UIColor *)speedLimitDarkGray __attribute__ ((deprecated));

+ (UIColor *)colorWithName:(NSString *)colorName __attribute__ ((deprecated));
+ (UIColor *)colorFromHexString:(NSString *)hexString;

+ (void)setNightMode:(BOOL)mode;
+ (BOOL)isNightMode;

- (UIColor *)opposite __attribute__ ((deprecated));

@end

NS_ASSUME_NONNULL_END
