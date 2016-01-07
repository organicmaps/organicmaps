NS_CLASS_AVAILABLE_IOS(8_0) @interface MWMPageController : UIPageViewController

+ (instancetype)pageControllerWithParent:(UIViewController *)parentViewController;
- (void)close;
- (void)nextPage;
- (void)show;

- (void)enableFirst:(UIButton *)button;
- (void)enableSecond;
- (void)skipFirst;
- (void)skipSecond;

@end
