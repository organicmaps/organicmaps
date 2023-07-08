#import "SwizzleStyle.h"
#import <UIKit/UIKit.h>
#import "objc/runtime.h"
#import "objc/message.h"

@implementation SwizzleStyle

+ (void)swizzle
{
  [SwizzleStyle swizzle:[UISearchBar class] methodName:@"didMoveToWindow"];
  [SwizzleStyle swizzle:[UITextField class] methodName:@"didMoveToWindow"];
  [SwizzleStyle swizzle:[UIView class] methodName:@"didMoveToWindow"];
}

+ (void)swizzle:(Class)forClass methodName:(NSString*)methodName
{
  SEL originalMethod = NSSelectorFromString(methodName);
  SEL newMethod = NSSelectorFromString([NSString stringWithFormat:@"%@%@", @"sw_", methodName]);
  [SwizzleStyle swizzle:forClass from:originalMethod to:newMethod];
}

+ (void)swizzle:(Class)forClass from:(SEL)original to:(SEL)new
{
  Method originalMethod = class_getInstanceMethod(forClass, original);
  Method newMethod = class_getInstanceMethod(forClass, new);
  if (class_addMethod(forClass, original, method_getImplementation(newMethod), method_getTypeEncoding(newMethod))) {
    class_replaceMethod(forClass, new, method_getImplementation(originalMethod), method_getTypeEncoding(originalMethod));
  } else {
    method_exchangeImplementations(originalMethod, newMethod);
  }
}

@end
