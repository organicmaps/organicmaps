#import "MWMBottomMenuView.h"
#import "MWMViewController.h"

#include "platform/location.hpp"

@class MapViewController, MWMButton;

@protocol MWMBottomMenuControllerProtocol<NSObject>

- (void)actionDownloadMaps;
- (void)closeInfoScreens;

@end

@interface MWMBottomMenuViewController : MWMViewController

@property(nonatomic) MWMBottomMenuState state;
@property(weak, nonatomic) IBOutlet MWMButton * p2pButton;
@property(nonatomic) CGFloat leftBound;

- (instancetype)initWithParentController:(MapViewController *)controller
                                delegate:(id<MWMBottomMenuControllerProtocol>)delegate;

- (void)setStreetName:(NSString *)streetName;
- (void)setInactive;
- (void)setPlanning;
- (void)setGo;

- (void)refreshLayout;

- (void)processMyPositionStateModeEvent:(location::EMyPositionMode)mode;

@end
