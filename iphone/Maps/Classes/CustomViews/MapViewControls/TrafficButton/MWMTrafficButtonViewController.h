#import "MWMViewController.h"

@interface MWMTrafficButtonViewController : MWMViewController

+ (MWMTrafficButtonViewController *)controller;

@property(nonatomic) BOOL hidden;

+ (void)updateAvailableArea:(CGRect)frame;

@end
