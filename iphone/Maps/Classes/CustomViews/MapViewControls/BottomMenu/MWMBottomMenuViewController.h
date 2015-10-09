
#import "MWMBottomMenuView.h"

#include "platform/location.hpp"

@class MapViewController;

@protocol MWMBottomMenuControllerProtocol<NSObject>

- (void)actionDownloadMaps;
- (void)closeInfoScreens;

@end

@interface MWMBottomMenuViewController : UIViewController

@property(nonatomic) MWMBottomMenuState state;
@property(weak, nonatomic) IBOutlet UIButton * p2pButton;
@property(nonatomic) CGFloat leftBound;

- (instancetype)initWithParentController:(MapViewController *)controller
                                delegate:(id<MWMBottomMenuControllerProtocol>)delegate;

- (void)onEnterForeground;

- (void)setStreetName:(NSString *)streetName;
- (void)setInactive;
- (void)setPlanning;
- (void)setGo;

- (void)refreshLayout;

- (void)onLocationStateModeChanged:(location::EMyPositionMode)state;

@end
