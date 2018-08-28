#import "MWMBottomMenuView.h"

@class MapViewController, MWMButton;
@protocol MWMBottomMenuControllerProtocol;

@interface MWMBottomMenuViewController : UIViewController

+ (MWMBottomMenuViewController *)controller;

@property(nonatomic) MWMBottomMenuState state;

- (instancetype)initWithParentController:(MapViewController *)controller
                                delegate:(id<MWMBottomMenuControllerProtocol>)delegate;

- (void)mwm_refreshUI;
- (void)updateBadgeVisible:(BOOL)visible;

+ (void)updateAvailableArea:(CGRect)frame;

@property(weak, readonly, nonatomic) MWMButton * searchButton;
@property(weak, readonly, nonatomic) MWMButton * routeButton;
@property(weak, readonly, nonatomic) MWMButton * discoveryButton;
@property(weak, readonly, nonatomic) MWMButton * bookmarksButton;
@property(weak, readonly, nonatomic) MWMButton * moreButton;

@end
