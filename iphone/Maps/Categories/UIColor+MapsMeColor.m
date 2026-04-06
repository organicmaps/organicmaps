#import "UIColor+MapsMeColor.h"

static UIColor * NamedColor(NSString * name)
{
  return [UIColor colorNamed:name inBundle:NSBundle.mainBundle compatibleWithTraitCollection:nil];
}

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

+ (UIColor *)greenPrimary
{
  return NamedColor(@"greenPrimary");
}

+ (UIColor *)fadeBackgroundColor
{
  return NamedColor(@"fadeBackground");
}

+ (UIColor *)pressBackground
{
  return NamedColor(@"pressBackground");
}

+ (UIColor *)redPrimary
{
  return NamedColor(@"redPrimary");
}

+ (UIColor *)linkBlue
{
  return NamedColor(@"linkBlue");
}

+ (UIColor *)linkBlueHighlighted
{
  return NamedColor(@"linkBlueHighlighted");
}

+ (UIColor *)linkBlueDarkColor
{
  return NamedColor(@"linkBlueDark");
}
+ (UIColor *)buttonRed
{
  return NamedColor(@"buttonRed");
}
+ (UIColor *)blackPrimaryText
{
  return NamedColor(@"blackPrimaryText");
}

+ (UIColor *)blackSecondaryText
{
  return NamedColor(@"blackSecondaryText");
}

+ (UIColor *)blackHintText
{
  return NamedColor(@"blackHintText");
}

+ (UIColor *)blackDividers
{
  return NamedColor(@"blackDividers");
}

+ (UIColor *)whitePrimary
{
  return NamedColor(@"whitePrimary");
}

+ (UIColor *)whitePrimaryText
{
  return NamedColor(@"whitePrimaryText");
}

+ (UIColor *)whitePrimaryTextHighlighted
{
  return NamedColor(@"whitePrimaryTextHighlighted");
}

+ (UIColor *)whiteHintText
{
  return NamedColor(@"whiteHintText");
}

+ (UIColor *)buttonDisabledBlueText
{
  return NamedColor(@"buttonDisabledBlueText");
}

+ (UIColor *)buttonHighlightedBlueTextColor
{
  return NamedColor(@"buttonHighlightedBlueText");
}

+ (UIColor *)blackOpaque
{
  return NamedColor(@"blackOpaque");
}

+ (UIColor *)carplayPlaceholderBackgroundColor
{
  return NamedColor(@"carplayPlaceholderBackground");
}

+ (UIColor *)bookingBackground
{
  return NamedColor(@"bookingBackground");
}

+ (UIColor *)opentableBackground
{
  return NamedColor(@"opentableBackground");
}

+ (UIColor *)transparentGreen
{
  return NamedColor(@"transparentGreen");
}

+ (UIColor *)speedLimitRed
{
  return NamedColor(@"speedLimitRed");
}

+ (UIColor *)speedLimitGreen
{
  return NamedColor(@"speedLimitGreen");
}

+ (UIColor *)speedLimitWhiteColor
{
  return NamedColor(@"speedLimitWhite");
}

+ (UIColor *)speedLimitLightGrayColor
{
  return NamedColor(@"speedLimitLightGray");
}

+ (UIColor *)speedLimitDarkGrayColor
{
  return NamedColor(@"speedLimitDarkGray");
}
@end
