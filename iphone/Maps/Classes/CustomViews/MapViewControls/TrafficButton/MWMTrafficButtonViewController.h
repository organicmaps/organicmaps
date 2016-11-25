@interface MWMTrafficButtonViewController : UIViewController

+ (MWMTrafficButtonViewController *)controller;

@property(nonatomic) BOOL hidden;
@property(nonatomic) CGFloat leftBound;

- (void)mwm_refreshUI;

@end
