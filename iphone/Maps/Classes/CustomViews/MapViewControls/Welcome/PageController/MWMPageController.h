#import "MWMWelcomeController.h"

NS_CLASS_AVAILABLE_IOS(8_0) @interface MWMPageController : UIPageViewController

+ (instancetype)pageControllerWithParent:(UIViewController *)parentViewController;

- (void)close;
- (void)nextPage;
- (void)show:(Class<MWMWelcomeControllerProtocol>)welcomeClass;

@end
