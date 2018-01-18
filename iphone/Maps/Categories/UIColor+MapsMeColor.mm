#import "UIColor+MapsMeColor.h"

namespace
{

CGFloat const alpha04 = 0.04;
CGFloat const alpha12 = 0.12;
CGFloat const alpha26 = 0.26;
CGFloat const alpha30 = 0.3;
CGFloat const alpha32 = 0.32;
CGFloat const alpha36 = 0.36;
CGFloat const alpha40 = 0.4;
CGFloat const alpha54 = 0.54;
CGFloat const alpha70 = 0.7;
CGFloat const alpha80 = 0.8;
CGFloat const alpha87 = 0.87;
CGFloat const alpha90 = 0.9;
CGFloat const alpha100 = 1.;

BOOL isNightMode = NO;

CGFloat scaled(CGFloat f) { return f / 255.; }
NSDictionary<NSString *, UIColor *> * night = @{
  @"primaryDark":
      [UIColor colorWithRed:scaled(25.) green:scaled(30) blue:scaled(35.) alpha:alpha100],
  @"primary": [UIColor colorWithRed:scaled(45.) green:scaled(50.) blue:scaled(55.) alpha:alpha100],
  // Light green color
  @"primaryLight":
      [UIColor colorWithRed:scaled(65.) green:scaled(70.) blue:scaled(75.) alpha:alpha100],
  @"menuBackground":
      [UIColor colorWithRed:scaled(45.) green:scaled(50.) blue:scaled(55.) alpha:alpha80],
  @"downloadBadgeBackground":
      [UIColor colorWithRed:scaled(230.) green:scaled(70.) blue:scaled(60.) alpha:alpha100],
  // Background color && press color
  @"pressBackground":
      [UIColor colorWithRed:scaled(50.) green:scaled(54.) blue:scaled(58.) alpha:alpha100],
  // Red color (use for status closed in place page)
  @"red": [UIColor colorWithRed:scaled(230.) green:scaled(70.) blue:scaled(60.) alpha:alpha100],
  @"errorPink":
      [UIColor colorWithRed:scaled(246.) green:scaled(60.) blue:scaled(51.) alpha:alpha26],
  // Orange color (use for status 15 min in place page)
  @"orange": [UIColor colorWithRed:scaled(250.) green:scaled(190.) blue:scaled(10.) alpha:alpha100],
  // Blue color (use for links and phone numbers)
  @"linkBlue":
      [UIColor colorWithRed:scaled(80.) green:scaled(195.) blue:scaled(240.) alpha:alpha100],
  @"linkBlueHighlighted":
      [UIColor colorWithRed:scaled(60.) green:scaled(155.) blue:scaled(190.) alpha:alpha30],
  @"linkBlueDark":
      [UIColor colorWithRed:scaled(75.) green:scaled(185.) blue:scaled(230.) alpha:alpha100],
  @"buttonRed":
      [UIColor colorWithRed:scaled(244.) green:scaled(67.) blue:scaled(67.) alpha:alpha100],
  @"buttonRedHighlighted":
      [UIColor colorWithRed:scaled(183.) green:scaled(28.) blue:scaled(28.) alpha:alpha100],
  @"blackPrimaryText": [UIColor colorWithWhite:1. alpha:alpha90],
  @"blackSecondaryText": [UIColor colorWithWhite:1. alpha:alpha70],
  @"blackHintText": [UIColor colorWithWhite:1. alpha:alpha30],
  @"blackDividers": [UIColor colorWithWhite:1. alpha:alpha12],
  @"white": [UIColor colorWithRed:scaled(60.) green:scaled(64.) blue:scaled(68.) alpha:alpha100],
  @"whiteSecondaryText": [UIColor colorWithWhite:0. alpha:alpha70],
  @"whiteHintText": [UIColor colorWithWhite:0. alpha:alpha26],
  @"buttonDisabledBlueText":
      [UIColor colorWithRed:scaled(255.) green:scaled(230.) blue:scaled(140.) alpha:alpha30],
  @"alertBackground":
      [UIColor colorWithRed:scaled(60.) green:scaled(64.) blue:scaled(68.) alpha:alpha90],
  @"blackOpaque": [UIColor colorWithWhite:1. alpha:alpha04],
  @"bannerBackground":
      [UIColor colorWithRed:scaled(71.) green:scaled(75.) blue:scaled(80.) alpha:alpha100],
  @"bannerButtonBackground":
      [UIColor colorWithRed:scaled(45.) green:scaled(48.) blue:scaled(50.) alpha:alpha100],
  @"toastBackground": [UIColor colorWithWhite:0. alpha:alpha87],
  @"statusBarBackground": [UIColor colorWithWhite:0. alpha:alpha32]
};

NSDictionary<NSString *, UIColor *> * day = @{
  @"primaryDark" :
      [UIColor colorWithRed:scaled(24.) green:scaled(128) blue:scaled(68.) alpha:alpha100],
  @"primary" :
      [UIColor colorWithRed:scaled(32.) green:scaled(152.) blue:scaled(82.) alpha:alpha100],
  // Light green color
  @"primaryLight" :
      [UIColor colorWithRed:scaled(36.) green:scaled(180.) blue:scaled(98.) alpha:alpha100],
  @"menuBackground" : [UIColor colorWithWhite:1. alpha:alpha80],
  @"downloadBadgeBackground" :
      [UIColor colorWithRed:scaled(255.) green:scaled(55.) blue:scaled(35.) alpha:alpha100],
  // Background color && press color
  @"pressBackground" :
      [UIColor colorWithRed:scaled(245.) green:scaled(245.) blue:scaled(245.) alpha:alpha100],
  // Red color (use for status closed in place page)
  @"red" : [UIColor colorWithRed:scaled(230.) green:scaled(15.) blue:scaled(35.) alpha:alpha100],
  @"errorPink" :
      [UIColor colorWithRed:scaled(246.) green:scaled(60.) blue:scaled(51.) alpha:alpha12],
  // Orange color (use for status 15 min in place page)
  @"orange" : [UIColor colorWithRed:1. green:scaled(120.) blue:scaled(5.) alpha:alpha100],
  // Blue color (use for links and phone numbers)
  @"linkBlue" :
      [UIColor colorWithRed:scaled(30.) green:scaled(150.) blue:scaled(240.) alpha:alpha100],
  @"linkBlueHighlighted" :
      [UIColor colorWithRed:scaled(30.) green:scaled(150.) blue:scaled(240.) alpha:alpha30],
  @"linkBlueDark" :
      [UIColor colorWithRed:scaled(25.) green:scaled(135.) blue:scaled(215.) alpha:alpha100],
  @"buttonRed" :
      [UIColor colorWithRed:scaled(244.) green:scaled(67.) blue:scaled(67.) alpha:alpha100],
  @"buttonRedHighlighted" :
      [UIColor colorWithRed:scaled(183.) green:scaled(28.) blue:scaled(28.) alpha:alpha100],
  @"blackPrimaryText" : [UIColor colorWithWhite:0. alpha:alpha87],
  @"blackSecondaryText" : [UIColor colorWithWhite:0. alpha:alpha54],
  @"blackHintText" : [UIColor colorWithWhite:0. alpha:alpha26],
  @"blackDividers" : [UIColor colorWithWhite:0. alpha:alpha12],
  @"white" : [UIColor colorWithWhite:1. alpha:alpha100],
  @"whiteSecondaryText" : [UIColor colorWithWhite:1. alpha:alpha54],
  @"whiteHintText" : [UIColor colorWithWhite:1. alpha:alpha30],
  @"buttonDisabledBlueText" :
      [UIColor colorWithRed:scaled(3.) green:scaled(122.) blue:scaled(255.) alpha:alpha26],
  @"alertBackground" : [UIColor colorWithWhite:1. alpha:alpha90],
  @"blackOpaque" : [UIColor colorWithWhite:0. alpha:alpha04],
  @"bannerBackground" :
      [UIColor colorWithRed:scaled(252.) green:scaled(247.) blue:scaled(237.) alpha:alpha100],
  @"bannerButtonBackground" :
      [UIColor colorWithRed:scaled(242.) green:scaled(234.) blue:scaled(218.) alpha:alpha100],
  @"toastBackground" : [UIColor colorWithWhite:1. alpha:alpha87],
  @"statusBarBackground" : [UIColor colorWithWhite:1. alpha:alpha36]
};

UIColor * color(SEL cmd)
{
  return (isNightMode ? night : day)[NSStringFromSelector(cmd)];
}

} // namespace

@implementation UIColor (MapsMeColor)

+ (void)setNightMode:(BOOL)mode
{
  isNightMode = mode;
}

+ (BOOL)isNightMode
{
  return isNightMode;
}

- (UIColor *)opposite
{
  NSString * key = [(isNightMode ? day : night) allKeysForObject:self].firstObject;
  UIColor * color = (key == nil ? nil : (isNightMode ? night : day)[key]);
  return color ?: self;
}

// Dark green color
+ (UIColor *)primaryDark
{
  return color(_cmd);
}

// Green color
+ (UIColor *)primary
{
  return color(_cmd);
}

// Light green color
+ (UIColor *)primaryLight
{
  return color(_cmd);
}

// Use for opaque fullscreen
+ (UIColor *)fadeBackground
{
  return [UIColor colorWithWhite:0. alpha:alpha80];
}

+ (UIColor *)menuBackground
{
  return color(_cmd);
}

+ (UIColor *)downloadBadgeBackground
{
  return color(_cmd);
}
// Background color && press color
+ (UIColor *)pressBackground
{
  return color(_cmd);
}
// Red color (use for status closed in place page)
+ (UIColor *)red
{
  return color(_cmd);
}
// Yellow color (use for hotel's stars)
+ (UIColor *)yellow
{
  return [UIColor colorWithRed:scaled(255.) green:scaled(200.) blue:scaled(40.) alpha:alpha100];
}
// Green color (use for booking rating)
+ (UIColor *)green
{
  return [UIColor colorWithRed:scaled(85.) green:scaled(139.) blue:scaled(47.) alpha:alpha100];
}
// Pink background for invalid fields
+ (UIColor *)errorPink
{
  return color(_cmd);
}
// Orange color (use for status 15 min in place page)
+ (UIColor *)orange
{
  return color(_cmd);
}

// Blue color (use for links and phone numbers)
+ (UIColor *)linkBlue
{
  return color(_cmd);
}

+ (UIColor *)linkBlueHighlighted
{
  return color(_cmd);
}

+ (UIColor *)linkBlueDark
{
  return color(_cmd);
}
+ (UIColor *)buttonRed { return color(_cmd); }
+ (UIColor *)buttonRedHighlighted { return color(_cmd); }
+ (UIColor *)blackPrimaryText
{
  return color(_cmd);
}

+ (UIColor *)blackSecondaryText
{
  return color(_cmd);
}

+ (UIColor *)blackStatusBarBackground
{
  return [UIColor colorWithWhite:0. alpha:alpha40];
}

+ (UIColor *)blackHintText
{
  return color(_cmd);
}

+ (UIColor *)blackDividers
{
  return color(_cmd);
}

+ (UIColor *)white
{
  return color(_cmd);
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

+ (UIColor *)whiteSecondaryText
{
  return color(_cmd);
}

+ (UIColor *)whiteHintText
{
  return color(_cmd);
}

+ (UIColor *)whiteDividers
{
  return [UIColor colorWithWhite:1. alpha:alpha12];
}

+ (UIColor *)buttonDisabledBlueText
{
  return color(_cmd);
}

+ (UIColor *)buttonHighlightedBlueText
{
  return [UIColor colorWithRed:scaled(3.) green:scaled(122.) blue:scaled(255.) alpha:alpha54];
}

+ (UIColor *)alertBackground
{
  return color(_cmd);
}

+ (UIColor *)blackOpaque
{
  return color(_cmd);
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

+ (UIColor *)ratingRed
{
  return [UIColor colorWithRed:scaled(229) green:scaled(57) blue:scaled(53) alpha:alpha100];
}

+ (UIColor *)ratingOrange
{
  return [UIColor colorWithRed:scaled(244) green:scaled(81) blue:scaled(30) alpha:alpha100];
}

+ (UIColor *)ratingYellow
{
  return [UIColor colorWithRed:scaled(245) green:scaled(176) blue:scaled(39) alpha:alpha100];
}

+ (UIColor *)ratingLightGreen
{
  return [UIColor colorWithRed:scaled(124) green:scaled(179) blue:scaled(66) alpha:alpha100];
}

+ (UIColor *)ratingGreen
{
  return [UIColor colorWithRed:scaled(67) green:scaled(160) blue:scaled(71) alpha:alpha100];
}

+ (UIColor *)partner1Background
{
  return [UIColor colorWithRed:scaled(255) green:scaled(218) blue:scaled(0) alpha:alpha100];
}

+ (UIColor *)bannerBackground
{
  return color(_cmd);
}
+ (UIColor *)bannerButtonBackground { return color(_cmd); }
+ (UIColor *)toastBackground { return color(_cmd); }
+ (UIColor *)statusBarBackground { return color(_cmd); }
+ (UIColor *)colorWithName:(NSString *)colorName
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  return [[UIColor class] performSelector:NSSelectorFromString(colorName)];
#pragma clang diagnostic pop
}

@end
