#import "MWMWelcomeController.h"

@class MWMPageController;

@protocol MWMPageControllerProtocol <NSObject>

- (void)closePageController:(MWMPageController *)pageController;

@end

NS_CLASS_AVAILABLE_IOS(8_0) @interface MWMPageController : UIPageViewController

+ (instancetype)pageControllerWithParent:(UIViewController<MWMPageControllerProtocol> *)parentViewController welcomeClass:(Class<MWMWelcomeControllerProtocol>)welcomeClass;

- (void)close;
- (void)nextPage;
- (void)show;

@end
