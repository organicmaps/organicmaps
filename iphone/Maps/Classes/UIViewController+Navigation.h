@interface UIViewController (Navigation)

- (void)showBackButton;
- (void)backTap;

- (UIBarButtonItem *)navBarButtonWithImage:(UIImage *)image highlightedImage:(UIImage *)highlightedImage action:(SEL)action;
- (NSArray<UIBarButtonItem *> *)alignedNavBarButtonItems:(NSArray<UIBarButtonItem *> *)items;


@property (nonatomic, readonly) UIStoryboard * mainStoryboard;

@end
