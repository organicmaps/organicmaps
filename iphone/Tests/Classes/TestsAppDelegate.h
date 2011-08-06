#import <UIKit/UIKit.h>

@class TestsViewController;

@interface TestsAppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *window;
    TestsViewController *viewController;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet TestsViewController *viewController;

@end

