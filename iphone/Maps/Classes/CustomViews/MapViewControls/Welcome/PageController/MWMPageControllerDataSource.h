#import "MWMWelcomeController.h"

NS_CLASS_AVAILABLE_IOS(8_0) @interface MWMPageControllerDataSource : NSObject <UIPageViewControllerDataSource>

- (instancetype)initWithWelcomeClass:(Class<MWMWelcomeControllerProtocol>)welcomeClass;
- (MWMWelcomeController *)firstWelcomeController;
- (void)setPageController:(MWMPageController *)pageController;

@end
