#import "UIFont+MapsMeFonts.h"

static NSString * const kRegularFont = @"HelveticaNeue";
static NSString * const kMediumFont = @"HelveticaNeue-Medium";
static NSString * const kLightFont = @"HelveticaNeue-Light";
static NSString * const kBoldFont = @"HelveticaNeue-Bold";

@implementation UIFont (MapsMeFonts)

+ (UIFont *)regular10
{
  return [UIFont fontWithName:kRegularFont size:10];
}

+ (UIFont *)regular12
{
  return [UIFont fontWithName:kRegularFont size:12];
}

+ (UIFont *)regular14
{
  return [UIFont fontWithName:kRegularFont size:14];
}

+ (UIFont *)regular16
{
  return [UIFont fontWithName:kRegularFont size:16];
}

+ (UIFont *)regular17
{
  return [UIFont fontWithName:kRegularFont size:17];
}

+ (UIFont *)regular18
{
  return [UIFont fontWithName:kRegularFont size:18];
}

+ (UIFont *)regular24
{
  return [UIFont fontWithName:kRegularFont size:24];
}

+ (UIFont *)regular32
{
  return [UIFont fontWithName:kRegularFont size:32];
}

+ (UIFont *)regular52
{
  return [UIFont fontWithName:kRegularFont size:52];
}

+ (UIFont *)medium10
{
  return [UIFont fontWithName:kMediumFont size:10];
}
+ (UIFont *)medium14
{
  return [UIFont fontWithName:kMediumFont size:14];
}
+ (UIFont *)medium16
{
  return [UIFont fontWithName:kMediumFont size:16];
}
+ (UIFont *)medium17
{
  return [UIFont fontWithName:kMediumFont size:17];
}
+ (UIFont *)medium18
{
  return [UIFont fontWithName:kMediumFont size:18];
}
+ (UIFont *)medium24
{
  return [UIFont fontWithName:kMediumFont size:24];
}

+ (UIFont *)medium36
{
  return [UIFont fontWithName:kMediumFont size:36];
}

+ (UIFont *)medium40
{
  return [UIFont fontWithName:kMediumFont size:40.];
}

+ (UIFont *)medium44
{
  return [UIFont fontWithName:kMediumFont size:44.];
}

+ (UIFont *)light10
{
  return [UIFont fontWithName:kLightFont size:10];
}

+ (UIFont *)light12
{
  return [UIFont fontWithName:kLightFont size:12];
}

+ (UIFont *)light16
{
  return [UIFont fontWithName:kLightFont size:16];
}

+ (UIFont *)light17
{
  return [UIFont fontWithName:kLightFont size:17];
}

+ (UIFont *)bold16
{
  return [UIFont fontWithName:kBoldFont size:16];
}

+ (UIFont *)bold17
{
  return [UIFont fontWithName:kBoldFont size:17];
}

+ (UIFont *)bold48
{
  return [UIFont fontWithName:kBoldFont size:48];
}

+ (UIFont *)fontWithName:(NSString *)fontName
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  return [[UIFont class] performSelector:NSSelectorFromString(fontName)];
#pragma clang diagnostic pop
}

@end
