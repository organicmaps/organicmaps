#import "MWMCommon.h"

NSString * const kMediumFontName = @"HelveticaNeue-Medium";
NSString * const kLightFontName = @"HelveticaNeue-Light";

@implementation UIFont (MapsMeFonts)

+ (UIFont *)regular9 { return [UIFont systemFontOfSize:9]; }
+ (UIFont *)regular10 { return [UIFont systemFontOfSize:10]; }
+ (UIFont *)regular11 { return [UIFont systemFontOfSize:11]; }
+ (UIFont *)regular12 { return [UIFont systemFontOfSize:12]; }
+ (UIFont *)regular13 { return [UIFont systemFontOfSize:13]; }
+ (UIFont *)regular14 { return [UIFont systemFontOfSize:14]; }
+ (UIFont *)regular15 { return [UIFont systemFontOfSize:15]; }
+ (UIFont *)regular16 { return [UIFont systemFontOfSize:16]; }
+ (UIFont *)regular17 { return [UIFont systemFontOfSize:17]; }
+ (UIFont *)regular18 { return [UIFont systemFontOfSize:18]; }
+ (UIFont *)regular24 { return [UIFont systemFontOfSize:24]; }
+ (UIFont *)regular32 { return [UIFont systemFontOfSize:32]; }
+ (UIFont *)regular52 { return [UIFont systemFontOfSize:52]; }
+ (UIFont *)medium9 { return [UIFont systemFontOfSize:9 weight:UIFontWeightMedium]; }
+ (UIFont *)medium10 { return [UIFont systemFontOfSize:10 weight:UIFontWeightMedium]; }
+ (UIFont *)medium12 { return [UIFont systemFontOfSize:12 weight:UIFontWeightMedium]; }
+ (UIFont *)medium14 { return [UIFont systemFontOfSize:14 weight:UIFontWeightMedium]; }
+ (UIFont *)medium16 { return [UIFont systemFontOfSize:16 weight:UIFontWeightMedium]; }
+ (UIFont *)medium17 { return [UIFont systemFontOfSize:17 weight:UIFontWeightMedium]; }
+ (UIFont *)medium18 { return [UIFont systemFontOfSize:18 weight:UIFontWeightMedium]; }
+ (UIFont *)medium20 { return [UIFont systemFontOfSize:20 weight:UIFontWeightMedium]; }
+ (UIFont *)medium24 { return [UIFont systemFontOfSize:24 weight:UIFontWeightMedium]; }
+ (UIFont *)medium28 { return [UIFont systemFontOfSize:28 weight:UIFontWeightMedium]; }
+ (UIFont *)medium36 { return [UIFont systemFontOfSize:36 weight:UIFontWeightMedium]; }
+ (UIFont *)medium40 { return [UIFont systemFontOfSize:40 weight:UIFontWeightMedium]; }
+ (UIFont *)medium44 { return [UIFont systemFontOfSize:44 weight:UIFontWeightMedium]; }
+ (UIFont *)light10 { return [UIFont systemFontOfSize:10 weight:UIFontWeightLight]; }
+ (UIFont *)light12 { return [UIFont systemFontOfSize:12 weight:UIFontWeightLight]; }
+ (UIFont *)light16 { return [UIFont systemFontOfSize:16 weight:UIFontWeightLight]; }
+ (UIFont *)light17 { return [UIFont systemFontOfSize:17 weight:UIFontWeightLight]; }
+ (UIFont *)bold12 { return [UIFont boldSystemFontOfSize:12]; }
+ (UIFont *)bold14 { return [UIFont boldSystemFontOfSize:14]; }
+ (UIFont *)bold16 { return [UIFont boldSystemFontOfSize:16]; }
+ (UIFont *)bold17 { return [UIFont boldSystemFontOfSize:17]; }
+ (UIFont *)bold22 { return [UIFont boldSystemFontOfSize:22]; }
+ (UIFont *)bold24 { return [UIFont boldSystemFontOfSize:24]; }
+ (UIFont *)bold28 { return [UIFont boldSystemFontOfSize:28]; }
+ (UIFont *)bold36 { return [UIFont boldSystemFontOfSize:26]; }
+ (UIFont *)bold48 { return [UIFont boldSystemFontOfSize:48]; }
+ (UIFont *)italic16 { return [UIFont italicSystemFontOfSize:16]; }
+ (UIFont *)fontWithName:(NSString *)fontName
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  return [[UIFont class] performSelector:NSSelectorFromString(fontName)];
#pragma clang diagnostic pop
}

@end
