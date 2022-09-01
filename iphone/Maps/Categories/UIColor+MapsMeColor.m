#import "UIColorRoutines.h"
#import "UIColor+MapsMeColor.h"
#import "SwiftBridge.h"

static BOOL isNightMode = NO;
static NSDictionary<NSString *, UIColor *> *day;
static NSDictionary<NSString *, UIColor *> *night;

@implementation UIColor (MapsMeColor)

+ (void)load {
  day = @{
    @"primaryDark" :
        [UIColor colorWithRed:scaled(24.) green:scaled(128) blue:scaled(68.) alpha:alpha100],
    @"primary" :
        [UIColor colorWithRed:scaled(32.) green:scaled(152.) blue:scaled(82.) alpha:alpha100],
    @"secondary" : [UIColor colorWithRed:scaled(45) green:scaled(137) blue:scaled(83) alpha:alpha100],
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
    @"toastBackground" : [UIColor colorWithWhite:1. alpha:alpha87],
    @"statusBarBackground" : [UIColor colorWithWhite:1. alpha:alpha36],
    @"border" : [UIColor colorWithWhite:0. alpha:alpha04],
  };

  night = @{
    @"primaryDark":
        [UIColor colorWithRed:scaled(25.) green:scaled(30) blue:scaled(35.) alpha:alpha100],
    @"primary": [UIColor colorWithRed:scaled(45.) green:scaled(50.) blue:scaled(55.) alpha:alpha100],
    @"secondary": [UIColor colorWithRed:scaled(0x25) green:scaled(0x28) blue:scaled(0x2b) alpha:alpha100],
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
    @"toastBackground": [UIColor colorWithWhite:0. alpha:alpha87],
    @"statusBarBackground": [UIColor colorWithWhite:0. alpha:alpha32],
    @"border" : [UIColor colorWithWhite:1. alpha:alpha04],
  };
}

// hex string without #
+ (UIColor *)colorFromHexString:(NSString *)hexString
{
  unsigned rgbValue = 0;
  NSScanner *scanner = [NSScanner scannerWithString:hexString];
  [scanner setScanLocation:0];
  [scanner scanHexInt:&rgbValue];
  return [UIColor colorWithRed:((rgbValue & 0xFF0000) >> 16)/255.0 green:((rgbValue & 0xFF00) >> 8)/255.0 blue:(rgbValue & 0xFF)/255.0 alpha:1.0];
}

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

+ (UIColor *)speedLimitRed {
  return [UIColor colorWithRed:scaled(224) green:scaled(31) blue:scaled(31) alpha:alpha100];
}

+ (UIColor *)speedLimitGreen {
  return [UIColor colorWithRed:scaled(1) green:scaled(104) blue:scaled(44) alpha:alpha100];
}

+ (UIColor *)speedLimitWhite {
  return [UIColor colorWithRed:scaled(255) green:scaled(255) blue:scaled(255) alpha:alpha80];
}

+ (UIColor *)speedLimitLightGray {
  return [UIColor colorWithRed:scaled(0) green:scaled(0) blue:scaled(0) alpha:alpha20];
}

+ (UIColor *)speedLimitDarkGray {
  return [UIColor colorWithRed:scaled(51) green:scaled(51) blue:scaled(50) alpha:alpha100];
}
@end
