#import "SwizzleStyle.h"
#import "UIView+swizzle.h"

@implementation UIView (swizzle)
+(void)load {
  [SwizzleStyle swizzle:[self class] methodName:@"didMoveToWindow"];
}
@end
