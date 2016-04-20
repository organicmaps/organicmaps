@interface UIViewController (Navigation)

- (void)showBackButton;
- (void)backTap;

- (UIBarButtonItem *)buttonWithImage:(UIImage *)image action:(SEL)action;
- (NSArray<UIBarButtonItem *> *)alignedNavBarButtonItems:(NSArray<UIBarButtonItem *> *)items;

@property (nonatomic, readonly) UIStoryboard * mainStoryboard;

@end
