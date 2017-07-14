@interface MWMTrafficButtonViewController : UIViewController

+ (MWMTrafficButtonViewController *)controller;

@property(nonatomic) BOOL hidden;

- (void)mwm_refreshUI;

+ (void)updateAvailableArea:(CGRect)frame;

@end
