#import "MWMMapViewControlsManager.h"
#import "MWMPlacePageButtonsProtocol.h"
#include "geometry/point2d.hpp"

@class MWMViewController;

@protocol MWMActionBarProtocol<NSObject>

- (void)routeFrom;
- (void)routeTo;
- (void)routeAddStop;
- (void)routeRemoveStop;

- (void)share;

- (void)addBookmark;
- (void)removeBookmark;

- (void)call;
- (void)book;
- (void)searchBookingHotels;

- (void)openPartner;
- (void)downloadSelectedArea;
- (void)avoidToll;
- (void)avoidDirty;
- (void)avoidFerry;

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

- (void)show;
- (void)update;
- (void)showReview;
- (BOOL)isPPShown;
- (void)dismiss;
- (void)mwm_refreshUI;

@end
