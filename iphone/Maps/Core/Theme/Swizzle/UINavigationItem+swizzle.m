#import "UINavigationItem+swizzle.h"
#import "SwizzleStyle.h"

@implementation UINavigationItem (swizzle)
+(void)load {
  [SwizzleStyle swizzle:[self class] methodName:@"didMoveToWindow"];
}
@end
