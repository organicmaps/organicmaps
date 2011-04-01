#import <UIKit/UIKit.h>

@class SloynikSearchVC;

@interface AppDelegate : NSObject <UIApplicationDelegate>
{
  UIWindow * window;
  SloynikSearchVC * searchVC;
}

@property (nonatomic, retain) UIWindow * window;
@property (nonatomic, retain) SloynikSearchVC * searchVC;

@end
