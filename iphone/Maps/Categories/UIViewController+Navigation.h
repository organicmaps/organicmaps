@interface UIViewController (Navigation)

- (void)goBack;

- (UIBarButtonItem *)buttonWithImage:(UIImage *)image action:(SEL)action;
- (NSArray<UIBarButtonItem *> *)alignedNavBarButtonItems:(NSArray<UIBarButtonItem *> *)items;

@end
