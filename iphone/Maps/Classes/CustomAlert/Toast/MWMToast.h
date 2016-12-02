@interface MWMToast : NSObject

+ (void)showWithText:(NSString *)text;

+ (BOOL)affectsStatusBar;
+ (UIStatusBarStyle)preferredStatusBarStyle;

@end
