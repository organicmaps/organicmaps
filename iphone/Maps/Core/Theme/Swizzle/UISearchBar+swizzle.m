#import "UISearchBar+swizzle.h"
#import "SwizzleStyle.h"

@implementation UISearchBar (swizzle)
+(void)load {
  [SwizzleStyle swizzle:[self class] methodName:@"didMoveToWindow"];
}
@end
