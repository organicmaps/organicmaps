#import "UITextField+swizzle.h"
#import "SwizzleStyle.h"

@implementation UITextField (swizzle)
+(void)load {
  if (@available(iOS 12, *)) {
    [SwizzleStyle swizzle:[self class] methodName:@"didMoveToWindow"];
  }
}
@end
