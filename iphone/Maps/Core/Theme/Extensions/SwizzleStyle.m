#import "SwizzleStyle.h"
#import <UIKit/UIKit.h>
#import "objc/runtime.h"
#import "objc/message.h"

@implementation SwizzleStyle
+ (void)addSwizzle {
  [SwizzleStyle swizzle:[UIView class] methodName:@"didMoveToWindow"];

  [self swizzle:[UITextField class] methodName:@"textRectForBounds:"];
  [self swizzle:[UITextField class] methodName:@"editingRectForBounds:"];

  [SwizzleStyle swizzle:[UIWindow class] methodName:@"becomeKeyWindow"];
}

+ (void)swizzle:(Class)class methodName:(NSString*)methodName
{
  SEL originalMethod = NSSelectorFromString(methodName);
  SEL newMethod = NSSelectorFromString([NSString stringWithFormat:@"%@%@", @"sw_", methodName]);
  [SwizzleStyle swizzle:class from:originalMethod to:newMethod];
}

+ (void)swizzle:(Class)class from:(SEL)original to:(SEL)new
{
  Method originalMethod = class_getInstanceMethod(class, original);
  Method newMethod = class_getInstanceMethod(class, new);
  if (class_addMethod(class, original, method_getImplementation(newMethod), method_getTypeEncoding(newMethod))) {
    class_replaceMethod(class, new, method_getImplementation(originalMethod), method_getTypeEncoding(originalMethod));
  } else {
    method_exchangeImplementations(originalMethod, newMethod);
  }
}

@end
