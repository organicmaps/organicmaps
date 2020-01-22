#import "UIWindow+swizzle.h"
#import "SwizzleStyle.h"

@implementation UIWindow (swizzle)
+(void)load {
  [SwizzleStyle swizzle:[self class] methodName:@"becomeKeyWindow"];
}
@end
