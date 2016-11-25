#import "MWMBottomMenuView.h"
#import "MWMMapDownloaderTypes.h"
#import "MWMNavigationDashboardManager.h"

#include "platform/location.hpp"

@class MapViewController, MWMButton, MWMTaxiCollectionView;

@protocol MWMBottomMenuControllerProtocol<NSObject>

- (void)actionDownloadMaps:(mwm::DownloaderMode)mode;
- (void)closeInfoScreens;
- (void)addPlace:(BOOL)isBusiness hasPoint:(BOOL)hasPoint point:(m2::PointD const &)point;
- (void)didFinishAddingPlace;

@end

@interface MWMBottomMenuViewController : UIViewController<MWMNavigationDashboardInfoProtocol>

+ (MWMBottomMenuViewController *)controller;

@property(nonatomic) MWMBottomMenuState state;
@property(weak, nonatomic) IBOutlet MWMButton * p2pButton;
@property(nonatomic) CGFloat leftBound;
@property(nonatomic, readonly) CGFloat mainStateHeight;

- (instancetype)initWithParentController:(MapViewController *)controller
                                delegate:(id<MWMBottomMenuControllerProtocol>)delegate;

- (void)mwm_refreshUI;
- (MWMTaxiCollectionView *)taxiCollectionView;
- (void)setRoutingErrorMessage:(NSString *)routingErrorMessage;

@end
