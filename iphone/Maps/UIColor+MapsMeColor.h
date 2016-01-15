@interface UIColor (MapsMeColor)

+ (UIColor *)primaryDark;
+ (UIColor *)primary;
+ (UIColor *)primaryLight;
+ (UIColor *)fadeBackground;
+ (UIColor *)menuBackground;
+ (UIColor *)downloadBadgeBackground;
+ (UIColor *)pressBackground;
+ (UIColor *)red;
+ (UIColor *)orange;
+ (UIColor *)linkBlue;
+ (UIColor *)linkBlueDark;
+ (UIColor *)blackPrimaryText;
+ (UIColor *)blackSecondaryText;
+ (UIColor *)blackStatusBarBackground;
+ (UIColor *)blackHintText;
+ (UIColor *)blackDividers;
+ (UIColor *)white;
+ (UIColor *)whitePrimaryText;
+ (UIColor *)whiteSecondaryText;
+ (UIColor *)whiteDividers;
+ (UIColor *)buttonEnabledBlueText;
+ (UIColor *)buttonDisabledBlueText;
+ (UIColor *)buttonHighlightedBlueText;
+ (UIColor *)alertBackground;
+ (UIColor *)blackOpaque;

+ (UIColor *)colorWithName:(NSString *)colorName;

+ (void)setNightMode:(BOOL)mode;
+ (BOOL)isNightMode;

- (UIColor *)opposite;

@end
