#import "MWMWelcomeController.h"

@class MWMPageController;

@protocol MWMPageControllerProtocol <NSObject>

- (void)closePageController:(MWMPageController *)pageController;
- (void)mapSearchText:(NSString *)text forInputLocale:(NSString *)locale;

@end

NS_CLASS_AVAILABLE_IOS(8_0) @interface MWMPageController : UIPageViewController

+ (instancetype)pageControllerWithParent:(UIViewController<MWMPageControllerProtocol> *)parentViewController welcomeClass:(Class<MWMWelcomeControllerProtocol>)welcomeClass;

- (void)close;
- (void)nextPage;
- (void)show;
- (void)mapSearchText:(NSString *)text forInputLocale:(NSString *)locale;

@end
