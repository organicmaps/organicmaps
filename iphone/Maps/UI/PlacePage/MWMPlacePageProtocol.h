#import "MWMMapViewControlsManager.h"
#import "MWMPlacePageButtonsProtocol.h"

#include "Framework.h"

@class MWMViewController;

@protocol MWMActionBarProtocol<NSObject>

- (void)routeFrom;
- (void)routeTo;

- (void)share;

- (void)addBookmark;
- (void)removeBookmark;

- (void)call;
- (void)book:(BOOL)isDecription;

- (void)apiBack;
- (void)downloadSelectedArea;

@end

struct FeatureID;

@protocol MWMFeatureHolder<NSObject>

- (FeatureID const &)featureId;

@end

namespace booking
{
struct HotelFacility;
}

@protocol MWMBookingInfoHolder<NSObject>

- (std::vector<booking::HotelFacility> const &)hotelFacilities;
- (NSString *)hotelName;

@end

@protocol MWMPlacePageProtocol<MWMActionBarProtocol, MWMPlacePageButtonsProtocol, MWMFeatureHolder, MWMBookingInfoHolder>

@property(nonatomic) CGFloat topBound;
@property(nonatomic) CGFloat leftBound;

- (void)show:(place_page::Info const &)info;
- (void)close;
- (void)mwm_refreshUI;
- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation;
- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator;
- (void)addSubviews:(NSArray *)views withNavigationController:(UINavigationController *)controller;

@end
