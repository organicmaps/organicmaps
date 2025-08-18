#import "SwiftBridge.h"
#import "UIColor+MapsMeColor.h"
#import "UIColorRoutines.h"

static BOOL isNightMode = NO;

@implementation UIColor (MapsMeColor)

// hex string without #
+ (UIColor *)colorFromHexString:(NSString *)hexString
{
  unsigned rgbValue = 0;
  NSScanner * scanner = [NSScanner scannerWithString:hexString];
  [scanner setScanLocation:0];
  [scanner scanHexInt:&rgbValue];
  return [UIColor colorWithRed:((rgbValue & 0xFF0000) >> 16) / 255.0
                         green:((rgbValue & 0xFF00) >> 8) / 255.0
                          blue:(rgbValue & 0xFF) / 255.0
                         alpha:1.0];
}

+ (void)setNightMode:(BOOL)mode
{
  isNightMode = mode;
}

+ (BOOL)isNightMode
{
  return isNightMode;
}

// Green color
+ (UIColor *)primary
{
  return StyleManager.shared.theme.colors.primary;
}

// Use for opaque fullscreen
+ (UIColor *)fadeBackground
{
  return [UIColor colorWithWhite:0. alpha:alpha80];
}

// Background color && press color
+ (UIColor *)pressBackground
{
  return StyleManager.shared.theme.colors.pressBackground;
}
// Red color (use for status closed in place page)
+ (UIColor *)red
{
  return StyleManager.shared.theme.colors.red;
}

// Blue color (use for links and phone numbers)
+ (UIColor *)linkBlue
{
  return StyleManager.shared.theme.colors.linkBlue;
}

+ (UIColor *)linkBlueHighlighted
{
  return StyleManager.shared.theme.colors.linkBlueHighlighted;
}

+ (UIColor *)linkBlueDark
{
  return StyleManager.shared.theme.colors.linkBlueDark;
}
+ (UIColor *)buttonRed
{
  return StyleManager.shared.theme.colors.buttonRed;
}
+ (UIColor *)blackPrimaryText
{
  return StyleManager.shared.theme.colors.blackPrimaryText;
}

+ (UIColor *)blackSecondaryText
{
  return StyleManager.shared.theme.colors.blackSecondaryText;
}

+ (UIColor *)blackHintText
{
  return StyleManager.shared.theme.colors.blackHintText;
}

+ (UIColor *)blackDividers
{
  return StyleManager.shared.theme.colors.blackDividers;
}

+ (UIColor *)white
{
  return StyleManager.shared.theme.colors.white;
}

+ (UIColor *)whitePrimaryText
{
  return [UIColor colorWithWhite:1. alpha:alpha87];
}

+ (UIColor *)whitePrimaryTextHighlighted
{
  // use only for highlighted colors!
  return [UIColor colorWithWhite:1. alpha:alpha30];
}

+ (UIColor *)whiteHintText
{
  return StyleManager.shared.theme.colors.whiteHintText;
}

+ (UIColor *)buttonDisabledBlueText
{
  return StyleManager.shared.theme.colors.buttonDisabledBlueText;
}

+ (UIColor *)buttonHighlightedBlueText
{
  return [UIColor colorWithRed:scaled(3.) green:scaled(122.) blue:scaled(255.) alpha:alpha54];
}

+ (UIColor *)blackOpaque
{
  return StyleManager.shared.theme.colors.blackOpaque;
}

+ (UIColor *)carplayPlaceholderBackground
{
  return StyleManager.shared.theme.colors.carplayPlaceholderBackground;
}

+ (UIColor *)bookingBackground
{
  return [UIColor colorWithRed:scaled(25.) green:scaled(69.) blue:scaled(125.) alpha:alpha100];
}

+ (UIColor *)opentableBackground
{
  return [UIColor colorWithRed:scaled(218.) green:scaled(55) blue:scaled(67) alpha:alpha100];
}

+ (UIColor *)transparentGreen
{
  return [UIColor colorWithRed:scaled(233) green:scaled(244) blue:scaled(233) alpha:alpha26];
}

+ (UIColor *)colorWithName:(NSString *)colorName
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  return [[UIColor class] performSelector:NSSelectorFromString(colorName)];
#pragma clang diagnostic pop
}

+ (UIColor *)speedLimitRed
{
  return [UIColor colorWithRed:scaled(224) green:scaled(31) blue:scaled(31) alpha:alpha100];
}

+ (UIColor *)speedLimitGreen
{
  return [UIColor colorWithRed:scaled(1) green:scaled(104) blue:scaled(44) alpha:alpha100];
}

+ (UIColor *)speedLimitWhite
{
  return [UIColor colorWithRed:scaled(255) green:scaled(255) blue:scaled(255) alpha:alpha80];
}

+ (UIColor *)speedLimitLightGray
{
  return [UIColor colorWithRed:scaled(0) green:scaled(0) blue:scaled(0) alpha:alpha20];
}

+ (UIColor *)speedLimitDarkGray
{
  return [UIColor colorWithRed:scaled(51) green:scaled(51) blue:scaled(50) alpha:alpha100];
}
@end
