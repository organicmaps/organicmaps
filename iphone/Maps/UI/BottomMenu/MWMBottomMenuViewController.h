#import "MWMBottomMenuView.h"

@class MapViewController, MWMButton;
@protocol MWMBottomMenuControllerProtocol;

@interface MWMBottomMenuViewController : UIViewController

+ (MWMBottomMenuViewController *)controller;

@property(nonatomic) MWMBottomMenuState state;

- (instancetype)initWithParentController:(MapViewController *)controller
                                delegate:(id<MWMBottomMenuControllerProtocol>)delegate;

- (void)mwm_refreshUI;

+ (void)updateAvailableArea:(CGRect)frame;

@end
