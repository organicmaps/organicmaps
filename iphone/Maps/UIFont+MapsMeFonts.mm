#import "Common.h"
#import "UIFont+MapsMeFonts.h"

NSString * const kMediumFontName = @"HelveticaNeue-Medium";
NSString * const kLightFontName = @"HelveticaNeue-Light";

@implementation UIFont (MapsMeFonts)

+ (UIFont *)regular10
{
  return [UIFont systemFontOfSize:10];
}

+ (UIFont *)regular12
{
  return [UIFont systemFontOfSize:12];
}

+ (UIFont *)regular13
{
  return [UIFont systemFontOfSize:13];
}

+ (UIFont *)regular14
{
  return [UIFont systemFontOfSize:14];
}

+ (UIFont *)regular15
{
  return [UIFont systemFontOfSize:15];
}

+ (UIFont *)regular16
{
  return [UIFont systemFontOfSize:16];
}

+ (UIFont *)regular17
{
  return [UIFont systemFontOfSize:17];
}

+ (UIFont *)regular18
{
  return [UIFont systemFontOfSize:18];
}

+ (UIFont *)regular24
{
  return [UIFont systemFontOfSize:24];
}

+ (UIFont *)regular32
{
  return [UIFont systemFontOfSize:32];
}

+ (UIFont *)regular52
{
  return [UIFont systemFontOfSize:52];
}

+ (UIFont *)medium10
{
  CGFloat const size = 10;
  if (isIOS7)
    return [UIFont fontWithName:kMediumFontName size:size];
  return [UIFont systemFontOfSize:size weight:UIFontWeightMedium];
}
+ (UIFont *)medium14
{
  CGFloat const size = 14;
  if (isIOS7)
    return [UIFont fontWithName:kMediumFontName size:size];
  return [UIFont systemFontOfSize:size weight:UIFontWeightMedium];
}
+ (UIFont *)medium16
{
  CGFloat const size = 16;
  if (isIOS7)
    return [UIFont fontWithName:kMediumFontName size:size];
  return [UIFont systemFontOfSize:size weight:UIFontWeightMedium];
}

+ (UIFont *)medium17
{
  CGFloat const size = 17;
  if (isIOS7)
    return [UIFont fontWithName:kMediumFontName size:size];
  return [UIFont systemFontOfSize:size weight:UIFontWeightMedium];
}
+ (UIFont *)medium18
{
  CGFloat const size = 18;
  if (isIOS7)
    return [UIFont fontWithName:kMediumFontName size:size];
  return [UIFont systemFontOfSize:size weight:UIFontWeightMedium];
}

+ (UIFont *)medium20
{
  CGFloat const size = 20;
  if (isIOS7)
    return [UIFont fontWithName:kMediumFontName size:size];
  return [UIFont systemFontOfSize:size weight:UIFontWeightMedium];
}

+ (UIFont *)medium24
{
  CGFloat const size = 24;
  if (isIOS7)
    return [UIFont fontWithName:kMediumFontName size:size];
  return [UIFont systemFontOfSize:size weight:UIFontWeightMedium];
}

+ (UIFont *)medium28
{
  CGFloat const size = 28;
  if (isIOS7)
    return [UIFont fontWithName:kMediumFontName size:size];
  return [UIFont systemFontOfSize:size weight:UIFontWeightMedium];
}

+ (UIFont *)medium36
{
  CGFloat const size = 36;
  if (isIOS7)
    return [UIFont fontWithName:kMediumFontName size:size];
  return [UIFont systemFontOfSize:size weight:UIFontWeightMedium];
}

+ (UIFont *)medium40
{
  CGFloat const size = 40;
  if (isIOS7)
    return [UIFont fontWithName:kMediumFontName size:size];
  return [UIFont systemFontOfSize:size weight:UIFontWeightMedium];
}

+ (UIFont *)medium44
{
  CGFloat const size = 44;
  if (isIOS7)
    return [UIFont fontWithName:kMediumFontName size:size];
  return [UIFont systemFontOfSize:size weight:UIFontWeightMedium];
}

+ (UIFont *)light10
{
  CGFloat const size = 10;
  if (isIOS7)
    return [UIFont fontWithName:kLightFontName size:size];
  return [UIFont systemFontOfSize:size weight:UIFontWeightLight];
}

+ (UIFont *)light12
{
  CGFloat const size = 12;
  if (isIOS7)
    return [UIFont fontWithName:kLightFontName size:size];
  return [UIFont systemFontOfSize:size weight:UIFontWeightLight];
}

+ (UIFont *)light16
{
  CGFloat const size = 16;
  if (isIOS7)
    return [UIFont fontWithName:kLightFontName size:size];
  return [UIFont systemFontOfSize:size weight:UIFontWeightLight];
}

+ (UIFont *)light17
{
  CGFloat const size = 17;
  if (isIOS7)
    return [UIFont fontWithName:kLightFontName size:size];
  return [UIFont systemFontOfSize:size weight:UIFontWeightLight];
}

+ (UIFont *)bold12
{
  return [UIFont boldSystemFontOfSize:12];
}

+ (UIFont *)bold14
{
  return [UIFont boldSystemFontOfSize:14];
}

+ (UIFont *)bold16
{
  return [UIFont boldSystemFontOfSize:16];
}

+ (UIFont *)bold17
{
  return [UIFont boldSystemFontOfSize:17];
}

+ (UIFont *)bold48
{
  return [UIFont boldSystemFontOfSize:48];
}

+ (UIFont *)fontWithName:(NSString *)fontName
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  return [[UIFont class] performSelector:NSSelectorFromString(fontName)];
#pragma clang diagnostic pop
}

@end
