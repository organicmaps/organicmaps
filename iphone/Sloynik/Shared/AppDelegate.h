#import <UIKit/UIKit.h>

@class SearchVC;

@interface AppDelegate : NSObject <UIApplicationDelegate>
{
  UIWindow * window;
  SearchVC * searchVC;
}

@property (nonatomic, retain) UIWindow * window;
@property (nonatomic, retain) SearchVC * searchVC;

@end
